#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include "secret.h"

String githubUrl = "https://api.github.com/repos/" + githubUsername + "/" + githubRepository + "/releases/latest";

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  checkForNewRelease();
}

void loop() {
  ArduinoOTA.handle(); // Handle OTA updates in main loop
}

void checkForNewRelease() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(githubUrl);
    http.addHeader("User-Agent", "ESP32");

    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();

      DynamicJsonDocument doc(2048); // Adjust size as necessary
      DeserializationError error = deserializeJson(doc, payload);
      if (error) {
        Serial.println("Failed to parse JSON");
        return;
      }

      String latestVersion = doc["tag_name"];
      Serial.println("Current Version: " + currentVersion);
      Serial.println("Latest Version: " + latestVersion);

      if (latestVersion != currentVersion) {
        Serial.println("New release found, starting OTA update...");
        updateFirmware(latestVersion);
      } else {
        Serial.println("No new release found.");
      }
    } else {
      Serial.println("Error on HTTP request. Code: " + String(httpCode));
    }

    http.end();
  } else {
    Serial.println("WiFi not connected");
  }
}

void updateFirmware(String version) {
  String firmwareUrl = "https://github.com/" + githubUsername + "/" + githubRepository + "/releases/download/" + version + "/firmware.bin";

  HTTPClient http;
  http.begin(firmwareUrl);

  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    WiFiClient* stream = http.getStreamPtr();
    size_t contentLength = http.getSize();

    if (contentLength > 0) {
      Serial.println("Downloading and starting OTA update...");
      if (!Update.begin(contentLength)) {
        Serial.println("Not enough space for OTA update");
        return;
      }

      size_t written = Update.writeStream(*stream);
      if (written == contentLength) {
        Serial.println("OTA Update Successful");
      } else {
        Serial.println("OTA Update Failed");
      }

      if (Update.end()) {
        Serial.println("Restarting...");
        ESP.restart();
      } else {
        Serial.println("Update error: " + String(Update.getError()));
      }
    } else {
      Serial.println("Firmware not available");
    }
  } else {
    Serial.println("Failed to fetch firmware. HTTP Code: " + String(httpCode));
  }

  http.end();
}