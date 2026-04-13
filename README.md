# Smart Factory 5.0 - Portfolio Project

[PL] Witaj w repozytorium projektu **Smart Factory 5.0**. Jest to kompleksowa aplikacja klasy Enterprise, demonstrująca integrację systemów automatyki przemysłowej (OT) z chmurą i rozwiązaniami Edge AI za pomocą Node-RED i Mendixa.

[EN] Welcome to the **Smart Factory 5.0** repository. This is a comprehensive enterprise-grade project demonstrating the integration of Industrial Automation (OT) systems with Cloud and Edge AI solutions using Node-RED and Mendix.

---

## 🏗️ Repository Structure / Struktura Repozytorium

Detailed architecture plan / Szczegółowy plan architektury: [**Smart_Factory_5.0_Plan.md**](./Smart_Factory_5.0_Plan.md).

The following directories contain specific parts of the environment, code, and backups:
Poniższe katalogi zawierają poszczególne części środowiska, kod oraz kopie zapasowe (backupy):

*   **[ESP/](./ESP)** - Kod mikrokontrolera ESP32-S3 (PlatformIO) dla roli Edge AI i odczytu drgań z MPU6050. Zawiera również archiwum kodu (`backup/`).
*   **[Node-red/](./Node-red)** - Przepływy (flows) brzegowej bramki IoT Node-RED (agregacja, MQTT, OPC-UA, routing do Mendixa). Zawiera plik `flows.json`.
*   **[PLC/](./PLC)** - Logika automatyki przemysłowej, w tym kod dla S7-1500 (TIA Portal v19) oraz pliki zrzutów (`backup/`).
*   **[Mendix/](./Mendix)** - Pliki projektu webowego eksportowane z Mendix Studio Pro (MES/WMS, logika biznesowa, dashboardy).
*   **[MendixMobileApp/](./MendixMobileApp)** - Pliki projektu mobilnej aplikacji PWA Mendix przeznaczonej do zgłaszania usterek offline.

---

## 🎥 Demonstration Video / Wideo Demonstracyjne

Watch the full system in action here / Zobacz pełne działanie systemu:
> [**Smart Factory 5.0 - Full Demo**](https://youtu.be/SBSa59YZOQo)

---

## ⚙️ Architektura Systemu i Diagram Działania

Oto wysokopoziomowy diagram przepływu danych pomiędzy poszczególnymi warstwami architektury zgodnej z założeniami Przemysłu 4.0/5.0:

```mermaid
graph TD
    classDef hardware fill:#f9f,stroke:#333,stroke-width:2px;
    classDef gateway fill:#bbf,stroke:#333,stroke-width:2px;
    classDef cloud fill:#bfb,stroke:#333,stroke-width:2px;

    subgraph OT_Layer ["Warstwa OT (Hala Produkcyjna - Fizyczna)"]
        PLC["Sterowniki PLC<br>(Stan maszyn, Raporty produkcyjna)"]:::hardware
        Robot["Robot 6-osiowy<br>(Wykonywanie zadań)"]:::hardware
        ESP32["ESP32-S3 + Edge AI<br>(Analiza drgań MPU6050)"]:::hardware
        
        PLC --> Robot
    end

    subgraph Gateway_Layer ["Warstwa Edge Gateway"]
        NodeRed["Node-RED (IoT Gateway)<br>Agregacja, Throttling, RBE"]:::gateway
    end

    subgraph IT_Layer ["Warstwa IT / Chmura"]
        MendixCore["Mendix Backend<br>Logika, Tickety, Wyniki OEE"]:::cloud
        MendixUI["Mendix UI<br>PWA Offline, 3D Digital Twin"]:::cloud
    end

    %% Połączenia między warstwami
    PLC -- OPC UA / S7 --> NodeRed
    ESP32 -- MQTT (Telemetria i Awaria) --> NodeRed
    
    NodeRed -- API POST (Zdarzenia Krytyczne) --> MendixCore
    NodeRed -- MQTT (Dane do wykresów HMI) --> MendixCore
    MendixCore -- REST / MQTT Closed Loop --> NodeRed
    
    MendixCore --> MendixUI
```
