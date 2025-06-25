#include <SPI.h>
#include <ArduinoJson.h>
#include <communication.h>
#include <utils.h>

void setup() {
    Serial.begin(9600);
    while (!Serial);

    initLora(433E6, 10, 125E3, 6, 20, true);
}

void loop() {
    int packetSize = LoRa.parsePacket();
    if (packetSize > 0) {
        if (packetSize < sizeof(Readings*)) return;

        Readings receivedData;
        LoRa.readBytes((uint8_t*)&receivedData, sizeof(Readings));

        int rssi = LoRa.packetRssi();
        receivedData.signal = rssi;

        StaticJsonDocument<256> data;

        JsonObject orientation = data.createNestedObject("orientation");
        orientation["x"] = receivedData.orientation.x;
        orientation["y"] = receivedData.orientation.y;
        orientation["z"] = receivedData.orientation.z;

        JsonObject position = data.createNestedObject("position");
        position["x"] = receivedData.position.x;
        position["y"] = receivedData.position.y;
        position["z"] = receivedData.position.z;

        JsonObject acceleration = data.createNestedObject("acceleration");
        acceleration["x"] = receivedData.acceleration.x;
        acceleration["y"] = receivedData.acceleration.y;
        acceleration["z"] = receivedData.acceleration.z;

        data["temperature"] = receivedData.temperature;
        data["internal_temperature"] = receivedData.internal_temperature;
        data["pressure"] = receivedData.pressure;
        data["error"] = (uint8_t)receivedData.error;
        data["signal"] = receivedData.signal;

        serializeJson(data, Serial);
        Serial.println();
    }

    if (Serial.available() > 0) {
        static char buffer[MAX_MESSAGE_LENGTH + 1];
        size_t len = Serial.readBytesUntil('\n', buffer, MAX_MESSAGE_LENGTH);
        buffer[len] = '\0';

        LoRa.beginPacket();
        LoRa.write((uint8_t*)buffer, len);
        LoRa.endPacket();
    }
}
