#pragma once

Adafruit_BMP280 bmp;
TinyGPSPlus gps;
File file;
String finalFileName;

#define POWER_LED 3
#define SD_LED 4
#define BMP_LED 5
#define GPS_LED 6
#define LORA_LED 7