#include <FreeRTOS_SAMD21.h>
#include <SPI.h>
#include <WiFi101.h>
#include <ArduinoMqttClient.h>
#include "secrets.h"

const char ssid[] = SECRET_SSID;
const char pass[] = SECRET_PASS;

const char BROKER[] = "test.mosquitto.org";
const int MQTT_PORT = 1883;
const char TOPIC[] = "ufroicc452/grupo2";
const long INTERVAL = 250;
const int POTENTIOMETER_PIN = A1;

int wifiStatus = WL_IDLE_STATUS;
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

TaskHandle_t handlePotentiometerTask;

void delayMs(int ms) {
  vTaskDelay(pdMS_TO_TICKS(ms));
}

void setup() {
  Serial.begin(9600);

  while (!Serial) {}

  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("Wi-Fi shield is not present");
    while (true) {};
  }

  while (wifiStatus != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA/WPA2 network with SSID: ");
    Serial.println(ssid);
    wifiStatus = WiFi.begin(ssid, pass);

    Serial.print("Status: ");
    Serial.println(formatWifiStatus(wifiStatus));

    delay(1000);
  }

  printWiFiStatus();

  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(BROKER);

  if (!mqttClient.connect(BROKER, MQTT_PORT)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());

    while (true);
  }

  Serial.println("Successfully connected to the MQTT broker!");

  vSetErrorLed(LED_BUILTIN, HIGH);
  vSetErrorSerial(&Serial);

  xTaskCreate(potentiometerTask, "Potentiometer Task", 256, NULL, tskIDLE_PRIORITY + 1, &handlePotentiometerTask);

  vTaskStartScheduler();
}

const char *formatWifiStatus(int status) {
  switch (status) {
    case WL_IDLE_STATUS: return "WL_IDLE_STATUS";
    case WL_NO_SSID_AVAIL: return "WL_NO_SSID_AVAIL";
    case WL_SCAN_COMPLETED: return "WL_SCAN_COMPLETED";
    case WL_CONNECTED: return "WL_CONNECTED";
    case WL_CONNECT_FAILED: return "WL_CONNECT_FAILED";
    case WL_CONNECTION_LOST: return "WL_CONNECTION_LOST";
    case WL_DISCONNECTED: return "WL_DISCONNECTED";
    default: return "UNKNOWN_STATUS";
  }
}

void printWiFiStatus() {
  Serial.println("Wi-Fi connected!");

  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  long rssi = WiFi.RSSI();
  Serial.print("Signal Strength (RSSI): ");
  Serial.print(rssi);
  Serial.println(" dBm");
}

static void potentiometerTask(void *pvParameters) {
  pinMode(POTENTIOMETER_PIN, INPUT);
  Serial.println("Potentiometer task started");

  while (true) {
    mqttClient.poll();

    Serial.print("Sending a message! Topic: '");
    Serial.print(TOPIC);
    Serial.println("'");

    int value = analogRead(POTENTIOMETER_PIN);
    int angle = map(value, 0, 1023, 0, 180);

    Serial.print("Angle: ");
    Serial.println(angle);

    mqttClient.beginMessage(TOPIC);
    mqttClient.print("Angle: ");
    mqttClient.print(angle);
    mqttClient.endMessage();

    Serial.println();
    Serial.println();

    delayMs(INTERVAL);
  }
}

void loop() {}
