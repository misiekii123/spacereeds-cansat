#include <SPI.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <WiFiNINA.h>
#include <TinyGPS++.h>
#include <Arduino_LSM6DSOX.h>
#include <communication.h>
#include <data.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <utils.h>

void checkLeds() {
    const int delayTime = 150;

    for (int i = 0; i < 2; i++) {
        digitalWrite(POWER_LED, HIGH);
        delay(delayTime);
        digitalWrite(POWER_LED, LOW);
        delay(delayTime);
    }

    const int ledPins[] = {SD_LED, BMP_LED, GPS_LED, LORA_LED};
    const int ledCount = sizeof(ledPins) / sizeof(ledPins[0]);

    for (int cycle = 0; cycle < 2; cycle++) {
        for (int i = 0; i < ledCount; i++) {
            digitalWrite(ledPins[i], HIGH);
            delay(delayTime);
            digitalWrite(ledPins[i], LOW);
        }

        for (int i = ledCount - 1; i >= 0; i--) {
            digitalWrite(ledPins[i], HIGH);
            delay(delayTime);
            digitalWrite(ledPins[i], LOW);
        }
    }
}

void checkErrors(const Readings& data) {
    digitalWrite(SD_LED, LOW);
    digitalWrite(BMP_LED, LOW);
    digitalWrite(GPS_LED, LOW);
    digitalWrite(LORA_LED, LOW);

    if (data.error & SD_e) {
        digitalWrite(SD_LED, HIGH);
    }
    if (data.error & BMP_e) {
        digitalWrite(BMP_LED, HIGH);
    }
    if ((data.error & GPS_e) || (data.error & GPS_val_e)) {
        digitalWrite(GPS_LED, HIGH);
    }
//    if (data.error & LORA_e) {
//        digitalWrite(LORA_LED, HIGH);
//    }
}

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

void setup() {
    pinMode(LEDR, OUTPUT);
    pinMode(LEDG, OUTPUT);
    pinMode(LEDB, OUTPUT);
    pinMode(LORA_CS, OUTPUT);
    pinMode(SD_CS, OUTPUT);

    pinMode(POWER_LED, OUTPUT);
    pinMode(SD_LED, OUTPUT);
    pinMode(BMP_LED, OUTPUT);
    pinMode(GPS_LED, OUTPUT);
    pinMode(LORA_LED, OUTPUT);

    setRGB(LOW, LOW, LOW);
    Serial1.begin(9600);

    checkLeds();
    digitalWrite(POWER_LED, HIGH);

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

    checkErrors(data);
}