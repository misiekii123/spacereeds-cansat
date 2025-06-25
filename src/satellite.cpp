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

Adafruit_BMP280 bmp;
TinyGPSPlus gps;
File file;
String finalFileName;

bool bmp_ok = false;
bool imu_ok = false;
bool sd_ok = false;
bool lora_ok = false;

void checkLeds() {
    const int delayTime = 150;
    for (int i = 0; i < 2; i++) {
        digitalWrite(POWER_LED, HIGH);
        delay(delayTime);
        digitalWrite(POWER_LED, LOW);
        delay(delayTime);
    }

    const int ledPins[] = {SD_LED, BMP_LED, GPS_LED, LORA_LED};
    for (int cycle = 0; cycle < 2; cycle++) {
        for (int i = 0; i < 4; i++) {
            digitalWrite(ledPins[i], HIGH);
            delay(delayTime);
            digitalWrite(ledPins[i], LOW);
        }
        for (int i = 3; i >= 0; i--) {
            digitalWrite(ledPins[i], HIGH);
            delay(delayTime);
            digitalWrite(ledPins[i], LOW);
        }
    }
}

void checkErrors(const Readings& data) {
    digitalWrite(SD_LED,  (data.error & SD_e) ? HIGH : LOW);
    digitalWrite(BMP_LED, (data.error & BMP_e) ? HIGH : LOW);
    digitalWrite(GPS_LED, (data.error & GPS_e) ? HIGH : LOW);
    digitalWrite(LORA_LED, lora_ok ? LOW : HIGH);
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

void validateSensors() {
    bmp_ok = bmp.begin(0x76);
    imu_ok = IMU.begin();

    LoRaAccess(false);
    sd_ok = SD.begin(SD_CS);

    LoRaAccess(true);
    lora_ok = initLora(433E6, 10, 125E3, 6, 20, true);
    LoRaAccess(false);

    if (bmp_ok) {
        bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                        Adafruit_BMP280::SAMPLING_X2,
                        Adafruit_BMP280::SAMPLING_X16,
                        Adafruit_BMP280::FILTER_X16,
                        Adafruit_BMP280::STANDBY_MS_500);
    }
}

void readData(Readings& data) {
    if (!bmp_ok) data.error |= BMP_e;
    else {
        data.temperature = bmp.readTemperature();
        data.pressure = bmp.readPressure() / 100.0F;
    }

    bool gpsDataReceived = false;
    while (Serial1.available() > 0) {
        gps.encode(Serial1.read());
        gpsDataReceived = true;
    }

    if (gpsDataReceived) {
        data.position.x = (float)gps.location.lat();
        data.position.y = (float)gps.altitude.meters();
        data.position.z = (float)gps.location.lng();
    }

    if (!gpsDataReceived) data.error |= GPS_e;

    if (!imu_ok) {
        data.error |= IMU_e;
    } else {
        if (IMU.temperatureAvailable()) {
            IMU.readTemperature(data.internal_temperature);
        } else data.error |= IMU_e;

        if (IMU.accelerationAvailable()) {
            IMU.readAcceleration(data.acceleration.x, data.acceleration.y, data.acceleration.z);
        } else data.error |= IMU_e;

        if (IMU.gyroscopeAvailable()) {
            IMU.readGyroscope(data.orientation.x, data.orientation.y, data.orientation.z);
        } else data.error |= IMU_e;
    }

    if (!sd_ok || !SD.exists("/" + finalFileName)) data.error |= SD_e;
    if (!lora_ok) data.error |= LORA_e;
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
    Serial.begin(9600);

    checkLeds();
    digitalWrite(POWER_LED, HIGH);

    LoRaAccess(false);

    validateSensors();

    int missionNumber = getNextMissionNumber();
    finalFileName = String(missionNumber) + ".txt";
    if (sd_ok) {
        file = SD.open(finalFileName, FILE_WRITE);
        if (file) file.close();
    }

    LoRaAccess(true);
    initLora(433E6, 10, 125E3, 6, 20, true);
}

void loop() {
    validateSensors();

    Readings data = createReadings();
    readData(data);

    if (lora_ok) {
        LoRaAccess(true);
        sendData(data, sizeof(Readings));
        LoRaAccess(false);

        setRGB(LOW, HIGH, LOW);
        delay(20);
        setRGB(LOW, LOW, LOW);
    }

    StaticJsonDocument<256> json_data;
    jsonData(data, json_data);

    if (sd_ok) {
        file = SD.open(finalFileName, FILE_WRITE);
        if (file) {
            serializeJson(json_data, file);
            file.println();
            file.close();
        }
    }

    checkErrors(data);
}
