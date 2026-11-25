#include <WiFi.h>
#include <PubSubClient.h>
#include "esp_sleep.h"

// SmartBattery - comunicación Wokwi <-> aplicación Android (PWA)
// En Wokwi: instalar PubSubClient desde la sección Libraries.

const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASS = "";
const char* MQTT_HOST = "broker.hivemq.com";
const int MQTT_PORT = 1883;

// Debe ser EXACTAMENTE el mismo ID que aparece en la aplicación.
const char* DEVICE_ID = "demo01";

String rootTopic = String("smartbattery/") + DEVICE_ID;
String telemetryTopic = rootTopic + "/telemetry";
String configTopic = rootTopic + "/config";

const int PIN_BATTERY = 34;
const int LED_GREEN = 25;
const int LED_YELLOW = 26;
const int LED_RED = 27;
const int LED_OVER = 14;
const int BUTTON_INTERVAL = 33;

// true = simulación Wokwi. Publica cada 15 segundos y permite recibir
// cambios desde la app mientras la simulación está funcionando.
// false = modo de prueba de Deep Sleep (requiere circuito y alimentación adecuados).
#define DEMO_MODE true

RTC_DATA_ATTR int intervalHours = 3;
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
unsigned long lastPublishMs = 0;
const unsigned long DEMO_PUBLISH_INTERVAL_MS = 15000;

float readBatteryVoltage() {
  if (DEMO_MODE) return 12.65; // Voltaje ilustrativo; NO es una medición física.
  // Divisor de tensión: 47 kOhm arriba y 10 kOhm abajo.
  // GPIO34 NUNCA debe recibir directamente los 12 V del automóvil.
  float pinVolts = analogReadMilliVolts(PIN_BATTERY) / 1000.0;
  return pinVolts * 5.7;
}

String classify(float v) {
  if (v > 14.8) return "SOBRECARGA";
  if (v < 12.0) return "CRITICO";
  if (v < 12.4) return "REVISAR";
  return "BUEN_ESTADO";
}

void showLED(float v) {
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_OVER, LOW);

  if (v > 14.8) digitalWrite(LED_OVER, HIGH);
  else if (v < 12.0) digitalWrite(LED_RED, HIGH);
  else if (v < 12.4) digitalWrite(LED_YELLOW, HIGH);
  else digitalWrite(LED_GREEN, HIGH);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

  int pos = msg.indexOf("interval_hours");
  if (pos >= 0) {
    int colon = msg.indexOf(':', pos);
    int value = msg.substring(colon + 1).toInt();

    if (value == 1 || value == 3 || value == 6 || value == 8) {
      intervalHours = value;
      Serial.printf("Nuevo intervalo recibido desde la app: %d horas\n", intervalHours);
    } else {
      Serial.println("Intervalo ignorado: usa 1, 3, 6 u 8 horas.");
    }
  }
}

bool connectNetwork() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Conectando a WiFi Wokwi");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("No se pudo conectar a WiFi.");
    return false;
  }

  Serial.print("WiFi conectado. IP: ");
  Serial.println(WiFi.localIP());

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
  mqtt.setBufferSize(512);

  String clientId = String("SB-") + DEVICE_ID + "-" +
                    String((uint32_t)ESP.getEfuseMac(), HEX);

  Serial.println("Conectando al broker MQTT...");
  if (!mqtt.connect(clientId.c_str())) {
    Serial.printf("Fallo MQTT. Estado: %d\n", mqtt.state());
    return false;
  }

  mqtt.subscribe(configTopic.c_str());
  Serial.println("MQTT conectado. Escuchando configuracion de la app.");
  Serial.print("Tema de telemetria: "); Serial.println(telemetryTopic);
  Serial.print("Tema de configuracion: "); Serial.println(configTopic);
  return true;
}

void publishTelemetry() {
  float voltage = readBatteryVoltage();
  showLED(voltage);

  String payload = String("{\"device\":\"SmartBattery-01\",\"voltage\":") +
      String(voltage, 2) + ",\"state\":\"" + classify(voltage) +
      "\",\"interval_hours\":" + String(intervalHours) +
      ",\"connection\":\"WiFi\"}";

  if (mqtt.connected()) {
    bool ok = mqtt.publish(telemetryTopic.c_str(), payload.c_str(), false);
    Serial.println(ok ? "Telemetria enviada:" : "Error al publicar telemetria:");
    Serial.println(payload);
  } else {
    Serial.println("No hay conexion MQTT; telemetria no enviada.");
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_OVER, OUTPUT);
  pinMode(PIN_BATTERY, INPUT);
  pinMode(BUTTON_INTERVAL, INPUT_PULLUP);
  analogReadResolution(12);

  float voltage = readBatteryVoltage();
  showLED(voltage);
  Serial.printf("SmartBattery: %.2f V | %s\n", voltage, classify(voltage).c_str());

  bool online = connectNetwork();
  if (online) {
    publishTelemetry();
    lastPublishMs = millis();
  }

  if (!DEMO_MODE) {
    // En modo real se publica una vez y luego se apaga la radio.
    // El ESP32 no puede recibir cambios de la app mientras está en Deep Sleep.
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_OVER, LOW);

    mqtt.disconnect();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    esp_sleep_enable_ext0_wakeup(GPIO_NUM_33, 0);
    esp_sleep_enable_timer_wakeup((uint64_t)intervalHours * 3600ULL * 1000000ULL);
    Serial.printf("Entrando en Deep Sleep por %d horas.\n", intervalHours);
    esp_deep_sleep_start();
  }
}

void loop() {
  if (!DEMO_MODE) return;

  // En modo demo mantenemos MQTT activo para que la app pueda enviar
  // configuraciones en cualquier momento, no solo al arrancar el ESP32.
  if (WiFi.status() != WL_CONNECTED || !mqtt.connected()) {
    Serial.println("Reconectando WiFi/MQTT...");
    if (!connectNetwork()) {
      delay(2000);
      return;
    }
    publishTelemetry();
    lastPublishMs = millis();
  }

  mqtt.loop();

  if (millis() - lastPublishMs >= DEMO_PUBLISH_INTERVAL_MS) {
    publishTelemetry();
    lastPublishMs = millis();
  }

  delay(10);
}
