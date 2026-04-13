# Projekt Smart Factory 5.0 - Architektura IIoT (Plan Portfolio)

## 📌 Cel Projektu
Stworzenie zaawansowanej aplikacji klasy Enterprise demonstrującej praktyczną integrację systemów automatyki przemysłowej (OT) z nowoczesnymi systemami informatycznymi (IT), chmurą i rozwiązaniami Edge AI. Projekt ma udowodnić kompetencje na poziomie Mid/Senior Mendix Developer oraz Specjalisty ds. Integracji IT/OT.

---

## 🏗️ Architektura Systemu (Warstwy)

### 1. Warstwa Sprzętowa i Automatyki (OT - Hala Produkcyjna)
Tutaj rodzą się dane. Systemy działają niezależnie i na bieżąco kontrolują procesy fizyczne.

*   **Sterowniki PLC (3-4 sztuki):** Odpowiadają za kontrolę fizycznego procesu produkcyjnego. Raportują twarde dane: statusy (RUN/STOP), liczniki sztuk czy kody błędów. Jeden ze sterowników symuluje magazyn wewnątrz linii.
*   **Robot 6-osiowy:** Sterowany przez PLC, jego pozycja będzie wizualizowana w czasie rzeczywistym.
*   **Edge AI (ESP32-S3 + MPU6050):** Urządzenie brzegowe pełniące rolę inteligentnego czujnika. Wbudowany 6-osiowy akcelerometr i żyroskop mierzą m.in. drgania silnika. Model AI (wykonany w Edge Impulse, TinyML) analizuje drgania lokalnie w czasie rzeczywistym i wypluwa jedynie "Prawdopodobieństwo Awarii" unikając przeciążania sieci przesyłem surowych danych. Dodatkowa funkcja ręcznego zbierania paczki danych z aktualnego dnia w celu dodatkowej weryfikacji. 
Node red użyjemy do komunikacji z PLC sim advance, do ESP32-S3 możemy użyć innego protokołu.

### 2. Warstwa Przetwarzania Brzegowego (Edge Gateway)
*   **Node-RED:** Działa jako główny pośrednik (IoT Gateway) i tłumacz. Odbiera surowe dane, filtruje je i buduje z nich logiczne komunikaty dla ludzi.
    *   **Pobieranie danych (OPC UA):** Odbiera sygnały ze sterowników PLC korzystając ze standardu Przemysłu 4.0, czyli OPC UA (ewentualnie S7/Modbus, jako rezerwa) oraz z sieci czujników ESP32 (MQTT).
    *   **Czyszczenie, Agregacja i Throttling:** Przeprowadza filtrację i buforowanie danych. Używa węzłów opóźniających (np. blokuje krótkotrwałe piki wibracyjne poniżej 5 sekund), aby uniknąć tzw. "Alarm Flooding" (Zalewu Alarmów) zanim sygnał trafi do Mendixa.
    *   **Raportowanie przez wyjątki (RBE - Report by Exception):** Dzieli ruch na dwa strumienie:
        *   **Zdarzenia Krytyczne (Event-Driven - API POST):** Wysłanie ustandaryzowanego, przełożonego na ludzki format JSONa o strukturze: `LineID`, `MachineID`, `ComponentID` (np. Wrzeciono), `AlarmCode`, `Priority` oraz czytelny `Message`. Ten pakiet omija opóźnienia MQTT i uderza z maksymalnym priorytetem do REST API bazy Mendix.
        *   **Telemetria (Filtrowana - MQTT):** Cykliczna wysyłka uśrednionych danych (np. co 10-30 sekund) za pomocą lekkiego szyfrowanego protokołu MQTTS w formacie JSON do żyrych wykresów HMI.

### 3. Warstwa Logiki Biznesowej i Prezentacji (Mendix - Chmura IT)
Platforma Mendix służy jako główne centrum dowodzenia (Backend & Frontend), uwalniając się od "brudnej" komunikacji sprzętowej.

*   **Zarządzanie Produkcją i Magazynem (MES / WMS):**
    *   Aplikacja do ustalania sekwencji i zlecania wypuszczania określonych części z magazynu umieszczonego w środku linii produkcyjnej.
    *   **Asynchroniczna Pętla Zwrotna (Closed Loop):** Obustronna komunikacja (REST/MQTT) – Mendix wysyła komendę nadpisania rejestru (np. status "Oczekujące na zmianę") i zawiesza aktualizację UI do czasu uzyskania potwierdzenia z PLC/Node-RED ("Zaktualizowałem"), aby uniknąć desynchronizacji w razie przerw sieciowych.
*   **Digital Twin 3D (Cyfrowy Bliźniak) i Monitorowanie SVG:**
    *   Interaktywny podgląd linii i robota. W przypadku trudności z wydajnością modeli 3D wprowadzony zostanie dynamiczny odpowiednik 2D w technologii SVG (widok z góry), na bieżąco subskrybujący zmiany statusów (MQTT) maszyn (np. kolor żółty = ryzyko awarii z ESP32, zielony = OK, czerwony = błąd). Naciskam na 3D damy radę.
*   **Analityka Biznesowa (OEE) i Retencja Danych:**
    *   Automatyczne wyliczanie kluczowych wskaźników efektywności maszyn (OEE).
    *   **Mechanizmy utrzymania bazy (Scheduled Events):** Zastosowanie mechanizmów zrzucania zagregowanych wyników dziennych OEE na stałe (Persistable), przy jednoczesnym oczyszczaniu szczegółowych nagromadzeń telemetrii ("śmieci"), co chroni przed szybkim wyczerpaniem miejsca bazodanowego darmowej wersji powiązanej z Mendix Cloud.
*   **Moduł Konserwacji (Maintenance) - Offline First & Deduplikacja:**
    *   Natywna aplikacja na urządzenia mobilne (tablet/telefon) dla personelu Utrzymania Ruchu, pozwalająca na pobranie natywnych Mendixowych encji "MaintenanceTicket", a następnie oznaczanie statusu awarii w pełnym trybie offline głęboko w piwnicach fabryki. Kopia synchronizuje się asynchronicznie po odzyskaniu zasięgu. Zewnętrzne systemy (jak Jira) są aktualizowane przez Mendixa dopiero na samym końcu drogi (Event-Driven webhook), co nie łamie trybu offline pracy mechaników.
    *   **Inteligentna Deduplikacja Biletów:** Logika Mendixa tworząca tickety chroni się przed dublami. Microflow nie stworzy kolejnego zgłoszenia, dopóki w bazie (lub w aplikacji mechanika) wisi otwarty i nierozwiązany bilet na identyczny zestaw kluczy kompozytowych:  `MachineID` + `ComponentID` + `AlarmCode`. Mechanik pracujący nad lewym wrzecionem robota Fanuc nie zostanie zasypany po raz kolejny błędami z lewego wrzeciona, gdy ktoś inny skasuje na panelu operatorskim HMI PLC "reset", a usterka fizyczna na linii nadal trwa.

### 4. Integracje Enterprise, API i Bezpieczeństwo
Udowodnienie płynnego poruszania się w świecie integracji systemów klasy Enterprise i świadomości cyberbezpieczeństwa.

*   **Obsługa Brokera i Zabezpieczenia (Cybersecurity):**
    *   Zainstalowany i skonfigurowany broker MQTT (np. HiveMQ/Mosquitto) do którego bezpiecznie (uwierzytelnianie + MQTTS) łączą się zarówno subskrybent (Mendix) jak i publikujący komunikaty (Node-RED/ESP32).
*   **Consumed API (Pobieranie / Odbiór):**
    *   Zintegrowanie z zewnętrznym systemem magazynowym ERP (zapytanie GET). Gdy system zgłasza ryzyko awarii np. łożyska (Ticket Maintenance), następuje automatyczne sprawdzenie czy dana część jest na zewnętrznym magazynie.
*   **Published API (Wystawienie własnej zabezpieczonej usługi):**
    *   Wystawienie autoryzowanego (wymagany Token / API Key) endpointu POST, dedykowanego systemom zewnętrznym. Pozwala na ominięcie bufora MQTT w celach bezzwłocznego zaraportowania krytycznego błędu produkcyjnego odczytanego na sterowniku PLC do bazy aplikacji Mendix.
*   **System Zewnętrznych Powiadomień (Opcjonalnie):**
    *   Wysłanie automatycznego ostrzeżenia (np. przez MS Teams/Slack) w momencie krytycznego zatoru lub silnego obniżenia wskaźnika OEE.

---

## 🛠️ Plan Działania i Kroków w Przyszłości

1.  **Rozwój Sprzętu i Połączeń:**
    *   Polutowanie/podłączenie MPU6050 z ESP32-S3 (SDA: D8, SCL: D9, 3.3V, GND).
    *   Wgranie podstawowego szkieletu C++ (odczyt akcelerometru po I2C i test działania komunikacji).
2.  **Konfiguracja Node-RED i MQTT:**
    *   Utworzenie przepływu do obsługi filtracji danych telemetrii przez RBE.
    *   Odtworzenie paczek JSON wysyłanych do Mendix na dwa kanały (Szybkie Alerty POST i Telemetria MQTT).
3.  **Utworzenie Domain Model w Mendix:**
    *   Przygotowanie struktury tabelkowej persystentnej dla Alarmów (`Machine_Status`) i Wskaźników Telemetrii (`MachineTelemetry` jako `Non-Persistable` / z czyszczeniem).
    *   Konfiguracja usług REST (Published/Consumed) z uwzględnieniem Mapowania Danych (Import/Export mapping).
4.  **Budowa UI i Cyfrowego Bliźniaka:**
    *   Pulpity Nawigacyjne.
    *   Zaimplementowanie modelu 3D (za pomocą odpowiednich modułów/widżetów 3D) z podpiętą subskrypcją danych.
5.  **Offline Maintenance App (Mendix):**
    *   Wyodrębnienie profili nawigacji dla urządzeń mobilnych.
    *   Testy logiki PWA/Native Mobile w trybie odcięcia połączenia (tryb samolotowy).
