#include "ArduinoCommon.h"

void initSerial() {
  Serial.begin(9600);
  while (!Serial) {}
}

void delayMs(int ms) {
  vTaskDelay(pdMS_TO_TICKS(ms));
}

void connectWiFi(const char* ssid, const char* pass) {
    int wifiStatus = WL_IDLE_STATUS;

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

    Serial.println("Wi-Fi connected!");
    printWiFiStatus();
}

void connectMQTT(MqttClient& mqttClient, const char* broker, int port) {
    Serial.print("Attempting to connect to the MQTT broker: ");
    Serial.println(broker);

    if (!mqttClient.connect(broker, port)) {
        Serial.print("MQTT connection failed! Error code = ");
        Serial.println(mqttClient.connectError());

        while (true);
    }

    Serial.println("Successfully connected to the MQTT broker!");
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

const char* formatWifiStatus(int status) {
  switch (status) {
    case WL_IDLE_STATUS:    return "IDLE";
    case WL_NO_SSID_AVAIL:  return "NO_SSID_AVAIL";
    case WL_SCAN_COMPLETED: return "SCAN_COMPLETED";
    case WL_CONNECTED:      return "CONNECTED";
    case WL_CONNECT_FAILED: return "CONNECT_FAILED";
    case WL_CONNECTION_LOST:return "CONNECTION_LOST";
    case WL_DISCONNECTED:   return "DISCONNECTED";
    default:                return "UNKNOWN";
  }
}
