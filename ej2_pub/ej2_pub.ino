#include <SPI.h>
#include <ArduinoMqttClient.h>
#include <ArduinoCommon.h>
#include <secrets.h>

const char ssid[] = SECRET_SSID;
const char pass[] = SECRET_PASS;

const char BROKER[] = "test.mosquitto.org";
const int MQTT_PORT = 1883;
const char TOPIC[] = "ufroicc452/tvf";
const long INTERVAL = 1000;

int wifiStatus = WL_IDLE_STATUS;
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

TaskHandle_t handlePublisherTask;

int count = 0;

void setup() {
  initSerial();
  connectWiFi(ssid, pass);
  connectMQTT(mqttClient, BROKER, MQTT_PORT);

  vSetErrorLed(LED_BUILTIN, HIGH);
  vSetErrorSerial(&Serial);

  xTaskCreate(publisherTask, "Publisher Task", 256, NULL, tskIDLE_PRIORITY + 1, NULL);

  vTaskStartScheduler();
}

static void publisherTask(void *pvParameters) {
    initPublisher(TOPIC);

    while (true) {
        mqttClient.poll();
        sendMessage(mqttClient, TOPIC, count);
        delayMs(INTERVAL);
    }
}

void initPublisher(const char* topic) {
    Serial.println("Publisher task started");
    Serial.print("Publishing to topic: ");
    Serial.println(topic);
}

void sendMessage(MqttClient& client, const char* topic, int& counter) {
    Serial.print("Sending a message! Topic: '");
    Serial.print(topic);
    Serial.println("'");

    Serial.print("Hello ");
    Serial.println(counter);

    client.beginMessage(topic);
    client.print("Hello ");
    client.print(counter);
    client.endMessage();

    Serial.println();

    counter++;
}

void loop() {}
