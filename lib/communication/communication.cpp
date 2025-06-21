#include "communication.h"

bool initLora(long frequency, int spreadingFactor, long bandwidth, int codingRate, int txPower, bool boost) {
    if (!LoRa.begin(frequency)) {
        setRGB(HIGH, LOW, LOW);
        return false;
    }
    LoRa.setSpreadingFactor(spreadingFactor);
    LoRa.setSignalBandwidth(bandwidth);
    LoRa.setCodingRate4(codingRate);
    LoRa.setTxPower(txPower, boost);
    return true;
}

void sendData(Readings& data, size_t size) {
    LoRa.beginPacket();
    LoRa.write((uint8_t*)&data, size);
    LoRa.endPacket();
}

void setRGB(PinStatus r, PinStatus g, PinStatus b) {
    digitalWrite(LEDR, r);
    digitalWrite(LEDG, g);
    digitalWrite(LEDB, b);
}

void LoRaAccess(bool access_to_lora) {
    if(access_to_lora) {
        digitalWrite(LORA_CS, LOW);
        digitalWrite(SD_CS, HIGH);
    }
    else {
        digitalWrite(LORA_CS, HIGH);
        digitalWrite(SD_CS, LOW);
    }
}