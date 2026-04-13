# Przegląd Architektury Oprogramowania PLC

Szczegółowy opis bloków i struktur danych dla Smart Factory 5.0.

## 1. Bloki Danych (DBs)

### `DB_OPC_UA_Node` (Globalny DB)
Ten blok jest źródłem prawdy dla mostu IT/OT.
- `MachineStatus` (Int): 0-Wyłączony, 1-Praca, 2-Błąd, 3-Konserwacja.
- `PieceCount` (DInt): Całkowita liczba wyprodukowanych sztuk.
- `ProductionTarget` (DInt): Cel produkcyjny ustawiony z poziomu Mendix.
- `Robot_X`, `Robot_Y`, `Robot_Z` (Real): Aktualna pozycja dla Cyfrowego Bliźniaka.
- `ESP32_Vibe_Alert` (Bool): Sygnał otrzymany z Node-RED (Edge AI).

### `DB_Internal_State` (Globalny DB)
- `CycleStart` (Bool)
- `EmergencyStop` (Bool)
- `ManualOverride` (Bool)

## 2. Bloki Funkcyjne (FBs)

### `FB_Robot_Control` [FB10]
- **Wejścia**: `i_Enable`, `i_TargetPos`
- **Wyjścia**: `q_Busy`, `q_Done`, `q_Error`
- **Logika**: Obsługuje maszynę stanów dla ruchu robota i koordynację z transporterem.

### `FB_Alarm_Handler` [FB99]
- **Wejścia**: `i_Trigger`, `i_Severity`, `i_Message`
- **Logika**: Formatuje alarm dla `DB_OPC_UA_Node`, aby mógł zostać pobrany przez logikę "Report by Exception" w Node-RED.

## 3. Bloki Organizacyjne (OBs)

- **OB1 (Main)**: Wywołuje bloki logiczne FB w sekwencji.
- **OB30 (100ms Cyclic)**: Używany do regulatorów PID lub wysokoczęstotliwościowej telemetrii (jeśli wymagana do lokalnego monitoringu).
