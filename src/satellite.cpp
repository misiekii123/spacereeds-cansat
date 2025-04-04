#include <SPI.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <WiFiNINA.h>
#include <TinyGPS++.h>
#include <Arduino_LSM6DSOX.h>
#include <communication.h>
#include <SD.h>
#include <ArduinoJson.h>

Adafruit_BMP280 bmp;
TinyGPSPlus gps;
File file;
String finalFileName;

Readings createReadings() {
    Readings data;
    data.temperature = 0.0;
    data.pressure = 0.0;
    data.position.x = data.position.y = data.position.z = 0.0;
    data.acceleration.x = data.acceleration.y = data.acceleration.z = 0.0;
    data.orientation.x = data.orientation.y = data.orientation.z = 0.0;
    data.error = 0b0000;
    data.signal = 0;
    return data;
}

void readData(Readings& data) {
    if (!bmp.begin(0x76)) data.error |= BMP_e;
    else {
        data.temperature = bmp.readTemperature();
        data.pressure = bmp.readPressure() / 100.0F;
    }

    while (Serial1.available() > 0) {
        gps.encode(Serial1.read());
        if (gps.location.isValid()) {
            data.position.x = (float)gps.location.lat();
            data.position.y = (float)gps.altitude.meters();
            data.position.z = (float)gps.location.lng();
        }
        else data.error |= GPS_val_e;
    }
    if (!Serial1.available()) data.error |= GPS_e;

    if (IMU.temperatureAvailable()) {
        IMU.readTemperature(data.internal_temperature);
    }
    else data.error |= IMU_e;

    if (IMU.accelerationAvailable()) {
        IMU.readAcceleration(data.acceleration.x, data.acceleration.y, data.acceleration.z);
    }
    else data.error |= IMU_e;

    if (IMU.gyroscopeAvailable()) {
        IMU.readGyroscope(data.orientation.x, data.orientation.y, data.orientation.z);
    }
    else data.error |= IMU_e;

    if(!SD.exists("/" + finalFileName + ".TXT")) data.error |= SD_e;
}

int getNextMissionNumber() {
    int maxNumber = 0;
    File root = SD.open("/");

    if (!root) {
        return 1;
    }

    while (true) {
        File entry = root.openNextFile();
        if (!entry) {
            break;
        }

        String fileName = entry.name();
        entry.close();

        int dotIndex = fileName.lastIndexOf('.');
        if (dotIndex > 0) {
            String numberPart = fileName.substring(0, dotIndex);
            int num = numberPart.toInt();
            if (num > maxNumber) {
                maxNumber = num;
            }
        }
    }
    root.close();
    return maxNumber + 1;
}

void jsonData(Readings& data, StaticJsonDocument<256>& json_file) {
    JsonObject orientation = json_file.createNestedObject("orientation");
    orientation["x"] = data.orientation.x;
    orientation["y"] = data.orientation.y;
    orientation["z"] = data.orientation.z;

    JsonObject position = json_file.createNestedObject("position");
    position["x"] = data.position.x;
    position["y"] = data.position.y;
    position["z"] = data.position.z;

    JsonObject acceleration = json_file.createNestedObject("acceleration");
    acceleration["x"] = data.acceleration.x;
    acceleration["y"] = data.acceleration.y;
    acceleration["z"] = data.acceleration.z;

    json_file["temperature"] = data.temperature;
    json_file["internal_temperature"] = data.internal_temperature;
    json_file["pressure"] = data.pressure;
    json_file["error"] = (uint8_t)data.error;
}

void setup() {
    pinMode(LEDR, OUTPUT);
    pinMode(LEDG, OUTPUT);
    pinMode(LEDB, OUTPUT);
    pinMode(LORA_CS, OUTPUT);
    pinMode(SD_CS, OUTPUT);

    setRGB(LOW, LOW, LOW);
    Serial1.begin(9600);

    LoRaAccess(false);
    if(!SD.begin(SD_CS)) {
        setRGB(HIGH, LOW, HIGH);
        while (1);
    }

    int missionNumber = getNextMissionNumber();
    finalFileName = String(missionNumber) + ".txt";
    file = SD.open(finalFileName, FILE_WRITE);
    if(file) file.close();

    LoRaAccess(true);
    initLora(433E6, 10, 125E3, 6, 20, true);

    if (!bmp.begin(0x76)) {
        setRGB(HIGH, HIGH, LOW);
        while (1);
    }

    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                    Adafruit_BMP280::SAMPLING_X2,
                    Adafruit_BMP280::SAMPLING_X16,
                    Adafruit_BMP280::FILTER_X16,
                    Adafruit_BMP280::STANDBY_MS_500);

    if (!IMU.begin()) {
        setRGB(LOW, LOW, HIGH);
        while (1);
    }
}

void loop() {
    Readings data = createReadings();
    readData(data);

    LoRaAccess(true);
    sendData(data, sizeof(Readings));
    LoRaAccess(false);

    StaticJsonDocument<256> json_data;
    jsonData(data, json_data);

    file = SD.open(finalFileName, FILE_WRITE);
    if (file) {
        serializeJson(json_data, file);
        file.println();
        file.close();
    }

    file = File();

    setRGB(LOW, HIGH, LOW);
    delay(20);
    setRGB(LOW, LOW, LOW);
}