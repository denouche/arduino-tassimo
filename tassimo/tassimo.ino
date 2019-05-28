#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

#include <DNSServer.h>           //needed for WiFiManager
#include <ESP8266WebServer.h>    //needed for this program and for WiFiManager
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>

#include <CloudIoTCore.h>

#include "Credentials.h"
#include "iot-core.h"

const char* software_name = "tassimo";
const char* software_version = "1.1.9";

// Put here the fingerprint of above domain name HTTPS certificate
// iot.leveugle.net TLS SHA-1 fingerprint, issued on 2017-11-04
String API_HTTPS_FINGERPRINT = "8C 57 D9 00 29 F4 59 13 0D BF CF CC 7F 79 A5 5B 5E 96 FB 58";

ESP8266WebServer server(80);

const int GPIO_PIN_4 = 4;
const int GPIO_PIN_5 = 5;
const int GPIO_PIN_12_RELAY = 12;

byte mac[6];

void on() {
  digitalWrite(GPIO_PIN_12_RELAY, HIGH);
}

void off() {
  digitalWrite(GPIO_PIN_12_RELAY, LOW);
}

void open() {
    digitalWrite(GPIO_PIN_4, LOW);
    delay(300);
    digitalWrite(GPIO_PIN_4, HIGH);
}

void press() {
    digitalWrite(GPIO_PIN_5, HIGH);
    delay(300);
    digitalWrite(GPIO_PIN_5, LOW);
}

void handleCoffee() {
    on();
    delay(1000);
    open();
    delay(1000);
    press();
    delay(120000);
    off();
}

String info() {
    String jsonString = getInformations();
    Serial.println(jsonString);
    return jsonString;
}

void update() {
    String macString = String(mac[0], HEX) + ":" + String(mac[1], HEX) + ":" + String(mac[2], HEX) + ":" + String(mac[3], HEX) + ":" + String(mac[4], HEX) + ":" + String(mac[5], HEX);
    t_httpUpdate_return ret = ESPhttpUpdate.update(API_HOSTNAME, 80, API_BASE_URL + "/download?mac=" + macString);
    Serial.println(ret);
    switch(ret) {
        case HTTP_UPDATE_FAILED:
            Serial.println("HTTP_UPDATE_FAILD Error code " + String(ESPhttpUpdate.getLastError()));
            Serial.println("HTTP_UPDATE_FAILD Error " + String(ESPhttpUpdate.getLastErrorString().c_str()));
            break;
        case HTTP_UPDATE_NO_UPDATES:
            Serial.println("HTTP_UPDATE_NO_UPDATES");
            break;
        case HTTP_UPDATE_OK:
            Serial.println("HTTP_UPDATE_OK");
            break;
    }
}

String getInformations() {
    const size_t bufferSize = JSON_OBJECT_SIZE(6) + 250;
    DynamicJsonBuffer jsonBuffer(bufferSize);
    JsonObject& root = jsonBuffer.createObject();

    root["application"] = software_name;
    root["version"] = software_version;
    root["mac"] = String(mac[0], HEX) + ":" + String(mac[1], HEX) + ":" + String(mac[2], HEX) + ":" + String(mac[3], HEX) + ":" + String(mac[4], HEX) + ":" + String(mac[5], HEX);
    root["ssid"] = WiFi.SSID();
    root["ip"] = String(WiFi.localIP()[0]) + "." + String(WiFi.localIP()[1]) + "." + String(WiFi.localIP()[2]) + "." + String(WiFi.localIP()[3]);
    root["plateform"] = "esp8266";

    String jsonString;
    root.printTo(jsonString);
    return jsonString;
}

void informConnexionDone() {
    String jsonString = getInformations();

    HTTPClient http;
    http.begin(API_PROTOCOL + API_HOSTNAME + API_BASE_URL + "/devices/register", API_HTTPS_FINGERPRINT);
    http.addHeader("Authorization", API_AUTHORIZATION);
    http.addHeader("Content-Type", "application/json");

    int httpCode = http.POST(jsonString);
    http.end();
}

void setup ( void ) {
    Serial.begin(115200);
    Serial.println("setup - begin");

    pinMode(GPIO_PIN_4, OUTPUT);
    digitalWrite(GPIO_PIN_4, HIGH); // by default, the open/close switch is in serial with the optocouper, so transistor should be ON by default. Pull-up resistor.

    pinMode(GPIO_PIN_5, OUTPUT);
    digitalWrite(GPIO_PIN_5, LOW);  // by default, the start switch is in parallel with this optocouper, so transistor should be OFF by default. Pull-down resistor.

    pinMode(GPIO_PIN_12_RELAY, OUTPUT);
    digitalWrite(GPIO_PIN_12_RELAY, LOW); // by default, the start switch is in parallel with the relay, so transistor should be OFF by default. Pull-down resistor.

    WiFiManager wifiManager;
    wifiManager.autoConnect("AutoConnectAP");

    WiFi.macAddress(mac);

    Serial.println("");
    // Wait for connection
    while(WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nConnected!\n");

    setupCloudIoT();

    Serial.println("\n Connected to MQTT !\n");

    info();
    informConnexionDone();
    update();
    info();

}

void handleConfig(String payload) {
    Serial.println("New config received : "+payload);
}

void handleCommand(String payload) {
    if(payload == "on") {
        on();
    } else if(payload =="off") {
        off();
    } else if(payload == "coffee") {
        handleCoffee();
    } else if(payload == "open") {
        open();
    } else if(payload == "press") {
        press();
    }
}

unsigned long lastMillis = 0;

void loop() {
    mqttClient->loop();
    delay(10);  // <- fixes some issues with WiFi stability
    if (!mqttClient->connected()) {
        connect();
    }
    if (millis() - lastMillis > 180000) {
        lastMillis = millis();
        publishTelemetry(info());
    }
}

 
