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
const char* software_version = "0.9.23";

const char* www_username = "denouche";
const char* www_password = "denouche";

// Put here the fingerprint of above domain name HTTPS certificate
// iot.leveugle.net TLS SHA-1 fingerprint, expire on 2017-10-04
String API_HTTPS_FINGERPRINT = "BB 13 89 F4 B0 82 3F 1D 90 CE 92 FD E6 98 9C 14 94 C3 6A CA";

ESP8266WebServer server(80);

const int GPIO_PIN_1 = 4;
const int GPIO_PIN_2 = 5;

byte mac[6];

void handleOpen() {
    if(!server.authenticate(www_username, www_password)) {
        return server.requestAuthentication();
    }

    digitalWrite(GPIO_PIN_1, LOW);
    delay(300);
    digitalWrite(GPIO_PIN_1, HIGH);
    server.send(204);
}

void handlePress() {
    if(!server.authenticate(www_username, www_password)) {
        return server.requestAuthentication();
    }

    digitalWrite(GPIO_PIN_2, HIGH);
    delay(300);
    digitalWrite(GPIO_PIN_2, LOW);
    server.send(204);
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
    t_httpUpdate_return ret = ESPhttpUpdate.update("iot.leveugle.net", 80, "/api/download?mac=" + macString);
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
    const size_t bufferSize = JSON_OBJECT_SIZE(5) + 200;
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
    Serial.println("Fingerprint");
    Serial.println(API_HTTPS_FINGERPRINT);
    http.begin(API_BASE_URL + "/devices/register", API_HTTPS_FINGERPRINT);
    http.addHeader("Authorization", API_AUTHORIZATION);
    http.addHeader("Content-Type", "application/json");

    int httpCode = http.POST(jsonString);

    Serial.println("httpCode");
    Serial.println(String(httpCode));

    http.end();
}


void setup ( void ) {
    Serial.begin(115200);
    Serial.println("setup - begin");

    pinMode(GPIO_PIN_1, OUTPUT);
    digitalWrite(GPIO_PIN_1, HIGH); // by default, the open/close switch is in serial with the optocouper, so transistor should be ON by default. Pull-up resistor.

    pinMode(GPIO_PIN_2, OUTPUT);
    digitalWrite(GPIO_PIN_2, LOW);  // by default, the start switch is in parallel with this optocouper, so transistor should be OFF by default. Pull-down resistor.

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

    server.on("/open", handleOpen);
    server.on("/press", handlePress);
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


