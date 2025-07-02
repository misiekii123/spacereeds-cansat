#pragma once
#include <communication.h>
#include <ArduinoJson.h>
#include <SD.h>

int getNextMissionNumber();
void jsonData(Readings& data, StaticJsonDocument<512>& json_file);
