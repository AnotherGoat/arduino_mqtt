#pragma once
#include <WiFi101.h>  
#include <ArduinoMqttClient.h>
#include <FreeRTOS_SAMD21.h>

void initSerial();

void connectWiFi(const char* ssid, const char* pass);

void connectMQTT(MqttClient& mqttClient, const char* broker, int port);

void printWiFiStatus();

const char* formatWifiStatus(int status);

void delayMs(int ms);