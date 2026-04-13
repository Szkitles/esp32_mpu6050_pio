# PLC - Smart Factory 5.0 (Siemens TIA Portal / PLC Sim Advance)

Ten katalog zawiera dokumentację i szablony architektoniczne dla warstwy PLC projektu Smart Factory 5.0.

## 📌 Struktura Logiczna (TIA Portal)

Aby zachować czystość i skalowalność projektu, zaleca się stosowanie następującej konwencji numeracji i nazewnictwa bloków w TIA Portal:

| Zakres | Typ | Nazwa | Opis |
| :--- | :--- | :--- | :--- |
| **OB1** | Main | `Main_Cycle` | Główna pętla programu. |
| **OB100** | Startup | `Startup` | Logika inicjalizacji (Reset stanów, czyszczenie buforów). |
| **FB10** | Logic | `FB_Robot_Interface` | Sterowanie i status zwrotny dla 6-osiowego robota. |
| **FB20** | Logic | `FB_Warehouse_Sim` | Logika przesuwu części w/z symulowanego magazynu. |
| **FB30** | Logic | `FB_Conveyor` | Sterowanie silnikiem, filtrowanie czujników i śledzenie. |
| **DB1** | Data | `DB_Global_Data` | Statusy systemowe, liczniki i stałe. |
| **DB10** | Data | `DB_OPC_UA_Node` | **Kluczowe:** Tagi wystawione dla Node-RED i Mendix. |
| **FC1** | Utility | `FC_IO_Handler` | Skalowanie wejść analogowych i mapowanie wejść/wyjść. |

## 🔗 Integracja (Logika Closed Loop)

Zgodnie z [Smart Factory 5.0 Plan](../Smart_Factory_5.0_Plan.md), każda komenda z Mendixa powinna odbywać się według schematu:

1.  **Mendix** zapisuje `Command_Request` = 1 do `DB_OPC_UA_Node`.
2.  **PLC** wykrywa żądanie, wykonuje logikę i zapisuje `Command_Done` = 1.
3.  **Mendix** odczytuje `Command_Done`, resetuje UI i opcjonalnie zapisuje potwierdzenie (ACK).

## 🛠️ Symulacja

Użyj **PLC Sim Advance**, aby wystawić instancję jako serwer OPC UA.
- Adres IP: `192.168.0.10` (Cel)
- Maska podsieci: `255.255.255.0`
