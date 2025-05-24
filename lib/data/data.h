#pragma once
#include <communication.h>
#include <ArduinoJson.h>
#include <SD.h>

int getNextMissionNumber();
void jsonData(Readings& data, StaticJsonDocument<256>& json_file);
