#include "communication.h"

void initLora(long frequency, int spreadingFactor, long bandwidth, int codingRate, int txPower, bool boost) {
    if (!LoRa.begin(frequency)) {
        setRGB(HIGH, LOW, LOW);
        while (1);
    }
    LoRa.setSpreadingFactor(spreadingFactor);
    LoRa.setSignalBandwidth(bandwidth);
    LoRa.setCodingRate4(codingRate);
    LoRa.setTxPower(txPower, boost);
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