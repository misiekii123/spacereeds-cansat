#pragma once

#include <Arduino.h>
#include "LoRa.h"
#include "WiFiNINA.h"

#define SD_CS 9
#define LORA_CS 10

typedef struct {
    float x;
    float y;
    float z;
} Vector3;

typedef struct {
    Vector3 orientation;
    Vector3 position;
    Vector3 acceleration;
    float temperature;
    int internal_temperature;
    float pressure;
    uint8_t error;
    char message[60];
    int signal;
} Readings;

enum errors {
  GPS_e = 0b00001,
  LORA_e = 0b00010,
  IMU_e = 0b00100,
  SD_e = 0b01000,
  BMP_e = 0b10000
};

bool initLora(long frequency, int spreadingFactor, long bandwidth, int codingRate, int txPower, bool boost);
void sendData(Readings& data, size_t size);
void setRGB(PinStatus r, PinStatus g, PinStatus b);
void LoRaAccess(bool access_to_lora);