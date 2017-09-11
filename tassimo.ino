#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

#include <WiFiClient.h> // Not needed if no need to get WiFi.localIP
#include <DNSServer.h>           //needed for WiFiManager
#include <ESP8266WebServer.h>    //needed for this program and for WiFiManager
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>

#include "Credentials.h"

const char* software_name = "tassimo";
const char* software_version = "1.0.2";

const char* www_username = "denouche";
const char* www_password = "denouche";

// Put here the fingerprint of above domain name HTTPS certificate
// iot.leveugle.net TLS SHA-1 fingerprint, issued on 2017-09-05
String API_HTTPS_FINGERPRINT = "B9 8C A0 87 A8 94 C0 11 C6 45 53 30 8D F8 44 48 DA 60 08 C5";

ESP8266WebServer server(80);

const int GPIO_PIN_1 = 4;
const int GPIO_PIN_2 = 5;
const int GPIO_PIN_RELAY = 12;

byte mac[6];

void on() {
  digitalWrite(GPIO_PIN_RELAY, HIGH);
}

void handleOn() {
    if(!server.authenticate(www_username, www_password)) {
        return server.requestAuthentication();
    }
    on();
    server.send(204);
}

void off() {
  digitalWrite(GPIO_PIN_RELAY, LOW);
}

void handleOff() {
    if(!server.authenticate(www_username, www_password)) {
        return server.requestAuthentication();
    }
    off();
    server.send(204);
}

void open() {
    digitalWrite(GPIO_PIN_1, LOW);
    delay(300);
    digitalWrite(GPIO_PIN_1, HIGH);
}

void handleOpen() {
    if(!server.authenticate(www_username, www_password)) {
        return server.requestAuthentication();
    }
    open();
    server.send(204);
}

void press() {
    digitalWrite(GPIO_PIN_2, HIGH);
    delay(300);
    digitalWrite(GPIO_PIN_2, LOW);
}

void handlePress() {
    if(!server.authenticate(www_username, www_password)) {
        return server.requestAuthentication();
    }
    press();
    server.send(204);
}

void handleCoffee() {
    if(!server.authenticate(www_username, www_password)) {
        return server.requestAuthentication();
    }
    server.send(204);
    on();
    delay(300);
    open();
    delay(300);
    press();
    delay(120000);
    off();
}

void handleNotFound() {
	  server.send(404);
}

String info() {
    String jsonString = getInformations();
    Serial.println(jsonString);
    return jsonString;
}

void handleInfo() {
    String jsonString = info();
    server.send(200, "application/json", jsonString);
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

void handleUpdate() {
    update();
    server.send(204);
}

void handleDisconnect() {
    server.send(204);
    delay(200);
    WiFi.disconnect();
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

    pinMode(GPIO_PIN_1, OUTPUT);
    digitalWrite(GPIO_PIN_1, HIGH); // by default, the open/close switch is in serial with the optocouper, so transistor should be ON by default. Pull-up resistor.

    pinMode(GPIO_PIN_2, OUTPUT);
    digitalWrite(GPIO_PIN_2, LOW);  // by default, the start switch is in parallel with this optocouper, so transistor should be OFF by default. Pull-down resistor.

    pinMode(GPIO_PIN_RELAY, OUTPUT);
    digitalWrite(GPIO_PIN_RELAY, LOW); // by default, the start switch is in parallel with the relay, so transistor should be OFF by default. Pull-down resistor.

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

    info();
    informConnexionDone();
    update();
    info();

    server.on("/on", handleOn);
    server.on("/off", handleOff);
    server.on("/open", handleOpen);
    server.on("/press", handlePress);
    
    server.on("/coffee", handleCoffee); // global command

    server.on("/info", handleInfo);
    server.on("/status", handleInfo);

    server.on("/update", handleUpdate);
    server.on("/disconnect", handleDisconnect);

    server.onNotFound(handleNotFound);

    server.begin();
    Serial.println("HTTP server started");
}

void loop() {
    server.handleClient();
}


