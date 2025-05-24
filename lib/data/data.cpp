#include "data.h"

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


int getNextMissionNumber() {
    int maxNumber = 0;
    File root = SD.open("/");

    if (!root)
        return 1;

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