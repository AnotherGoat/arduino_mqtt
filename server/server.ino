#include <FreeRTOS_SAMD21.h>
#include <SPI.h>
#include <WiFi101.h>
#include <Servo.h>
#include "secrets.h"

const char ssid[] = SECRET_SSID;
const char pass[] = SECRET_PASS;

const int LED_PIN = 0;
const int SERVO_PIN = 2;

int wifiStatus = WL_IDLE_STATUS;
WiFiServer server(80);
Servo servo;

TaskHandle_t handleServerTask;
TaskHandle_t handleLedTask;
QueueHandle_t ledQueue;
TaskHandle_t handleServoTask;
QueueHandle_t servoQueue;

enum LedAction {
  TURN_ON,
  TURN_OFF,
  TOGGLE
};

struct LedRequest {
  LedAction action;
  uint32_t durationMs;
};

struct ServoRequest {
  uint8_t angle;
};

String formatLedRequest(const LedRequest &request) {
  String actionName;

  switch (request.action) {
    case TURN_ON:
      actionName = "TURN_ON";
      break;
    case TURN_OFF:
      actionName = "TURN_OFF";
      break;
    case TOGGLE:
      actionName = "TOGGLE";
      break;
    default:
      actionName = "UNKNOWN";
      break;
  }

  return "LedRequest(action=" + actionName + ", durationMs=" + String(request.durationMs) + ")";
}

String formatServoRequest(const ServoRequest &request) {
  return "ServoRequest(angle=" + String(request.angle) + ")";
}

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

  vSetErrorLed(LED_BUILTIN, HIGH);
  vSetErrorSerial(&Serial);

  ledQueue = xQueueCreate(5, sizeof(LedRequest));
  servoQueue = xQueueCreate(5, sizeof(ServoRequest));

  xTaskCreate(serverTask, "Server Task", 1024, &server, tskIDLE_PRIORITY + 3, &handleServerTask);
  xTaskCreate(ledTask, "LED Task", 256, NULL, tskIDLE_PRIORITY + 2, &handleLedTask);
  xTaskCreate(servoTask, "Servo Task", 256, NULL, tskIDLE_PRIORITY + 1, &handleServoTask);

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

static void serverTask(void *pvParameters) {
  WiFiServer *server = (WiFiServer *)pvParameters;
  WiFiClient client;
  String requestLine;
  server->begin();

  Serial.println("Server task started");

  while (true) {
    client = server->available();

    if (client) {
      Serial.print("Client connected: ");
      Serial.println(client.remoteIP());
      requestLine = "";

      while (client.connected()) {
        if (client.available()) {
          char character = client.read();

          if (character == '\r') {
            break;
          }

          requestLine += character;
        } else {
          delayMs(1);
        }
      }

      Serial.print("Request: ");
      Serial.println(requestLine);

      if (requestLine.startsWith("GET /led/on")) {
        handleLedOn(client, requestLine);
      } else if (requestLine.startsWith("GET /led/off")) {
        handleLedOff(client, requestLine);
      } else if (requestLine.startsWith("GET /led/toggle")) {
        handleLedToggle(client);
      } else if (requestLine.startsWith("GET /servo/move")) {
        handleServoMove(client, requestLine);
      } else {
        handleUnknown(client, requestLine);
      }

      client.stop();
      Serial.println("Client disconnected");
    } else {
      delayMs(1);
    }
  }
}

static void ledTask(void *pvParameters) {
  pinMode(LED_PIN, OUTPUT);
  bool ledState = LOW;
  LedRequest request;

  Serial.println("LED task started");

  while (true) {
    xQueueReceive(ledQueue, &request, portMAX_DELAY);
    Serial.print("LED request received: ");
    Serial.println(formatLedRequest(request));

    switch (request.action) {
      case TURN_ON:
        ledState = HIGH;
        digitalWrite(LED_PIN, HIGH);

        if (request.durationMs > 0) {
          delayMs(request.durationMs);
          digitalWrite(LED_PIN, LOW);
          ledState = LOW;
        }

        break;
      case TURN_OFF:
        ledState = LOW;
        digitalWrite(LED_PIN, LOW);

        if (request.durationMs > 0) {
          delayMs(request.durationMs);
          digitalWrite(LED_PIN, HIGH);
          ledState = HIGH;
        }

        break;
      case TOGGLE:
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState);
        break;
    }
  }
}

static void servoTask(void *pvParameters) {
  servo.attach(SERVO_PIN);
  ServoRequest request;

  Serial.println("Servo task started");

  while (true) {
    xQueueReceive(servoQueue, &request, portMAX_DELAY);
    Serial.print("Servo request received: ");
    Serial.println(formatServoRequest(request));

    servo.write(request.angle);
    delayMs(500);
  }
}

void handleLedOn(WiFiClient &client, String requestLine) {
  LedRequest request = { TURN_ON, 0 };
  request.durationMs = parseIntArg(requestLine, "ms", 0);
  sendLedRequest(request);
  sendOkMessage(client, "LED ON queued");
}

void handleLedOff(WiFiClient &client, String requestLine) {
  LedRequest request = { TURN_OFF, 0 };
  request.durationMs = parseIntArg(requestLine, "ms", 0);
  sendLedRequest(request);
  sendOkMessage(client, "LED OFF queued");
}

void handleLedToggle(WiFiClient &client) {
  LedRequest request = { TOGGLE, 0 };
  sendLedRequest(request);
  sendOkMessage(client, "LED TOGGLE queued");
}

void sendLedRequest(LedRequest &request) {
  if (xQueueSend(ledQueue, &request, pdMS_TO_TICKS(10)) != pdTRUE) {
    Serial.println("Request lost because LED queue is full");
  }
}

void handleServoMove(WiFiClient &client, String &requestLine) {
  String angleArg = parseArg(requestLine, "angle");

  if (angleArg.isEmpty()) {
    sendNotFoundMessage(client, "Missing 'angle' parameter");
    return;
  }

  uint8_t angle = constrain(angleArg.toInt(), 0, 180);

  ServoRequest request = { angle };
  sendServoRequest(request);
  sendOkMessage(client, "SERVO MOVE queued");
}

void sendServoRequest(ServoRequest &request) {
  if (xQueueSend(servoQueue, &request, pdMS_TO_TICKS(10)) != pdTRUE) {
    Serial.println("Request lost because servo queue is full");
  }
}

void handleUnknown(WiFiClient &client, String requestLine) {
  sendNotFoundMessage(client, "Unknown request " + requestLine);
}

String parseArg(const String &requestLine, const String &argName) {
  int questionMark = requestLine.indexOf('?');

  if (questionMark == -1) {
    return "";
  }

  int argPosition = requestLine.indexOf(argName + "=", questionMark);

  if (argPosition == -1) {
    return "";
  }

  int valueStart = argPosition + argName.length() + 1;
  int valueEnd = requestLine.indexOf('&', valueStart);

  if (valueEnd == -1) {
    valueEnd = requestLine.indexOf(' ', valueStart);
  }

  if (valueEnd == -1) {
    valueEnd = requestLine.length();
  }

  return requestLine.substring(valueStart, valueEnd);
}

uint32_t parseIntArg(const String &requestLine, const String &argName, uint32_t defaultValue) {
  String value = parseArg(requestLine, argName);
  return value.isEmpty() ? defaultValue : value.toInt();
}

void sendMessage(WiFiClient &client, String status, String message) {
  client.print("HTTP/1.1 ");
  client.println(status);
  client.println("Content-Type: text/plain");
  client.println("Connection: close");
  client.println();
  client.println(message);
  client.flush();
}

void sendOkMessage(WiFiClient &client, String message) {
  sendMessage(client, "200 OK", message);
}

void sendNotFoundMessage(WiFiClient &client, String message) {
  sendMessage(client, "404 Not Found", message);
}

void loop() {}
