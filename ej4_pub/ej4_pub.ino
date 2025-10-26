#include <SPI.h>
#include <ArduinoMqttClient.h>
#include <ArduinoCommon.h>
#include <secrets.h>
#include <Servo.h>

const char ssid[] = SECRET_SSID;
const char pass[] = SECRET_PASS;

const char BROKER[] = "test.mosquitto.org";
const int MQTT_PORT = 1883;
const char TOPIC[] = "ufroicc452/tvf/servo/position";

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);
Servo servo;

const int SERVO_PIN = 9;

TaskHandle_t handleSubscriberTask;

void setup() {
  initSerial();
  connectWiFi(ssid, pass);
  connectMQTT(mqttClient, BROKER, MQTT_PORT);

  servo.attach(SERVO_PIN);
  servo.write(90);

  vSetErrorLed(LED_BUILTIN, HIGH);
  vSetErrorSerial(&Serial);

  xTaskCreate(subscriberTask, "Subscriber Task", 256, NULL, tskIDLE_PRIORITY + 1, &handleSubscriberTask);

  vTaskStartScheduler();
}

static void subscriberTask(void *pvParameters) {
    initSubscriber(mqttClient, TOPIC);

    while (true) {
        processMessage(mqttClient);
        delayMs(10); 
    }
}

void initSubscriber(MqttClient& client, const char* topic) {
    Serial.print("Subscribing to topic: ");
    Serial.println(topic);
    client.subscribe(topic);

    Serial.print("Waiting for messages on topic ");
    Serial.print(topic);
    Serial.println("...");
}

void processMessage(MqttClient& client) {
  int messageSize = client.parseMessage();
  if (messageSize == 0) return;

  String topic = client.messageTopic();
  String payload = readPayload(client, messageSize);

  handleServoCommand(payload);
}

String readPayload(MqttClient& client, int messageSize) {
  Serial.print("Received a message! Topic: '");
  Serial.print(client.messageTopic());
  Serial.print("', Length: ");
  Serial.print(messageSize);
  Serial.println(" bytes");

  String msg;
  while (client.available()) {
    char c = client.read();
    msg += c;
  }

  Serial.print("Payload: ");
  Serial.println(msg);
  Serial.println();

  return msg;
}

void handleServoCommand(const String& msg) {
    String trimmed = msg;
    trimmed.trim();

    if (!isValidCommand(trimmed)) return;

    int angle = extractAngle(trimmed);
    if (angle == -1) return;

    moveServo(angle);
}

bool isValidCommand(const String& msg) {
    if (!msg.startsWith("angle")) {
        Serial.print("Invalid command: '");
        Serial.print(msg);
        Serial.println("' (missing 'angle').\n");
        return false;
    }

    if (msg.indexOf('=') == -1) {
        Serial.print("Invalid command: '");
        Serial.print(msg);
        Serial.println("' (missing '=').\n");
        return false;
    }

    return true;
}

int extractAngle(const String& msg) {
    int equalPos = msg.indexOf('=');
    String valueStr = msg.substring(equalPos + 1);
    valueStr.trim();

    for (unsigned int i = 0; i < valueStr.length(); i++) {
        if (!isDigit(valueStr[i])) {
            Serial.print("Invalid angle value: '");
            Serial.print(valueStr);
            Serial.println("' (not a number).\n");
            return -1;
        }
    }

    int angle = valueStr.toInt();
    if (angle < 0 || angle > 180) {
        Serial.print("Invalid angle: ");
        Serial.print(angle);
        Serial.println(" (expected 0–180).\n");
        return -1;
    }

    return angle;
}

void moveServo(int angle) {
    Serial.print("Moving servo to ");
    Serial.println(angle);
    servo.write(angle);
    Serial.println();
}

void loop() {}
