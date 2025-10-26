#include <SPI.h>
#include <WiFi101.h>
#include <ArduinoCommon.h>
#include <secrets.h>

const char ssid[] = SECRET_SSID;
const char pass[] = SECRET_PASS;

const char BROKER[] = "test.mosquitto.org";
const int MQTT_PORT = 1883;

const int NUM_BUILDINGS = 3;
const int NUM_FLOORS = 4;

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

TaskHandle_t handleTemperatureTask;

void setup() {
  initSerial();
  connectWiFi(ssid, pass);
  connectMQTT(mqttClient, BROKER, MQTT_PORT);

  vSetErrorLed(LED_BUILTIN, HIGH);
  vSetErrorSerial(&Serial);

  xTaskCreate(temperatureTask, "Temperature Task", 256, NULL, tskIDLE_PRIORITY + 1, &handleTemperatureTask);

  vTaskStartScheduler();
}

static void temperatureTask(void *pvParameters) {
    Serial.println("Temperature task started");

    while (true) {
        mqttClient.poll();
        publishFakeTemperature(mqttClient);
        delayMs(200); 
    }
}

void publishFakeTemperature(MqttClient& client) {
    int buildingNumber = random(1, NUM_BUILDINGS + 1); 
    int floorNumber = random(1, NUM_FLOORS + 1);    
    float temperature = nextFloatInRange(20.0, 30.0);

    char topic[64]; 
    sprintf(topic, "ufroicc452/tvf/building/%d/floor/%d/temperature", buildingNumber, floorNumber);

    client.beginMessage(topic);
    client.print(temperature, 2);
    client.endMessage();

    Serial.print("Published ");
    Serial.print(temperature, 2);
    Serial.print("°C to topic ");
    Serial.println(topic);
}


float nextFloatInRange(float min, float max) {
    return min + ((float)random(0, 1000) / 1000.0) * (max - min);
}

void loop() {}
