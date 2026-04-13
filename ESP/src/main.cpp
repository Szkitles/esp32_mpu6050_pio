#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>

// Deklaracje glowne SDK z wyeksportowanej przez uzytkownika C++ Library dla klasyfikacji (TinyML)
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"

// --- KONFIGURACJA SIECI WiFi i MQTT ---
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* mqtt_server = "192.168.31.180";

WiFiClient espClient;
PubSubClient client(espClient);

// Funkcja laczaca sie z siecia WiFi
void setup_wifi() {
    delay(10);
    ei_printf("\nLonczenie do %s\n", ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        ei_printf(".");
    }
    ei_printf("\nPolaczono z WiFi!\nAdres IP: %s\n", WiFi.localIP().toString().c_str());
}

// Funkcja odnawiajaca polaczenie z MQTT (Brokerem Mosquitto)
void reconnect() {
    while (!client.connected()) {
        ei_printf("Proba polaczenia z MQTT...\n");
        // Proba polaczenia (ID klienta: ESP32Client_SmartFactory)
        if (client.connect("ESP32Client_SmartFactory")) {
            ei_printf("Polaczono z brokerem MQTT!\n");
            // Po udanym polaczeniu mozesz wyslac wiadomosc testowa
            client.publish("factory/machine1/status", "ESP32 gotowe do diagnozy!");
        } else {
            ei_printf("Blad polaczenia z MQTT, kod: %d\n", client.state());
            ei_printf("Nastepna proba za 5 sekund...\n");
            delay(5000);
        }
    }
}

Adafruit_MPU6050 mpu;

#define I2C_SDA 8
#define I2C_SCL 9

// Identyczna czestotliwosc z faza rejestracji 
#define FREQUENCY_HZ        100
#define INTERVAL_MS         (1000 / (FREQUENCY_HZ + 1))

static unsigned long last_interval_ms = 0;

// Tablica zbierajaca probki na potrzeby modelu 
float features[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];
size_t feature_ix = 0;

// Funkcja zwrotna odbierajaca dla Klasyfikatora wycinek tablicy cech (Features buffer window)
int raw_feature_get_data(size_t offset, size_t length, float *out_ptr) {
    memcpy(out_ptr, features + offset, length * sizeof(float));
    return 0;
}

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10); 

    // Uruchomienie WiFi i MQTT
    setup_wifi();
    client.setServer(mqtt_server, 1883);

    Wire.begin(I2C_SDA, I2C_SCL);

    if (!mpu.begin(0x68, &Wire)) {
        ei_printf("Nie udalo sie znalezc ukladu MPU6050\n");
        // Mozna tu rowniez wyslac info o awarii sprzetu na dedykowany topic:
        // client.publish("factory/machine1/status", "AWARIA SENSORA MPU6050");
        while (1) delay(10);
    }

    // Mocniejsze zakresy dla wykrywania uderzen z anomalii
    mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
    mpu.setGyroRange(MPU6050_RANGE_2000_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_184_HZ);
    
    ei_printf("-------------------------------------------\n");
    ei_printf("SYSTEM DIAGNOSTYKI DRGAN 5.0 URUCHOMIONY...\n");
    ei_printf("-------------------------------------------\n");
    ei_printf("Oczekuje na zebranie pierwszego okna czasowego (tzw. okno kroku inferencji)\n\n");
}

void loop() {
    // 0. Utrzymanie polaczenia MQTT
    if (!client.connected()) {
        reconnect();
    }
    client.loop(); // Musi byc wywolywane zeby odbierac/wysylac pakiety

    // 1. Sekwencyjne zbieranie paczki w czestotliwosci 100 Hz az do uzbierania tablicy wejsciowej
    if (millis() > last_interval_ms + INTERVAL_MS) {
        last_interval_ms = millis();

        sensors_event_t a, g, temp;
        mpu.getEvent(&a, &g, &temp);

        // Zapelnianie struktury features w takim samym formacie co eksportowalismy:
        // accX, accY, accZ, gyrX, gyrY, gyrZ
        features[feature_ix++] = a.acceleration.x;
        features[feature_ix++] = a.acceleration.y;
        features[feature_ix++] = a.acceleration.z;
        features[feature_ix++] = g.gyro.x;
        features[feature_ix++] = g.gyro.y;
        features[feature_ix++] = g.gyro.z;

        // 2. Kiedy tablica zbierze probki ilosci wyliczonej przez Edge Impulse, analizujemy uderzenie:
        if (feature_ix >= EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
            feature_ix = 0; // Zresetuj licznik probek dla nastepnych wstrzasow (przeplot krokowy mozna by wdrozyc poprawiajac pamiec)
            
            ei_impulse_result_t result = { 0 };

            signal_t features_signal;
            features_signal.total_length = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
            features_signal.get_data = &raw_feature_get_data;

            // Uruchomienie mikro-modelu na sprzecie! (Uruchamiamy auto-klasyfikator)
            EI_IMPULSE_ERROR res = run_classifier(&features_signal, &result, false);
            
            if (res != 0) {
                ei_printf("Wystapil wewnetrzny blad AI: ERR klasyfikatora: %d\n", res);
                return;
            }

            // Wypisywanie wyników AI co kazde skomletowane okno (ok 1 sek. dla modelu default z chmury)
            ei_printf("---[ ZBIORCZY WYNIK DIAGNOZY MASZYNY ]---\n");
            
            for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
                float confidence = result.classification[ix].value;
                ei_printf(" %s:\t%.2f %%\n", result.classification[ix].label, confidence * 100.0);
                
                // Formatyzuj i wyslij wynik przez MQTT dla kazdej etykiety
                char topicContext[50];
                snprintf(topicContext, sizeof(topicContext), "factory/machine1/vibration/%s", result.classification[ix].label);
                
                char payloadContext[10];
                snprintf(payloadContext, sizeof(payloadContext), "%.2f", confidence);
                
                client.publish(topicContext, payloadContext);
                
                // Alert wizualny przy wykryciu ewidentnej usterki
                if (strcmp(result.classification[ix].label, "anomaly") == 0 && confidence > 0.8) {
                    ei_printf("\n>>> [ALARM SYSTEMOWY] <<< \n");
                    ei_printf("WYKRYTO KRYTYCZNA AWARIE LOZYSKA/DRGAN!\n");
                    ei_printf(">>> [=================] <<< \n\n");
                    
                    // Bezposredni trigger dla Node-RED / Mendix!
                    client.publish("factory/machine1/alarms", "KRYTYCZNE DRGANIA (ANOMALIA) > 80%!");
                }
            }
            ei_printf("\n");
        }
    }
}
