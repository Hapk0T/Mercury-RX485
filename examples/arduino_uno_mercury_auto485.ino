#include <mercury.h>

// Arduino UNO + автоконвертер RS-485 (без линий управления DE/RE).
// modules switch direction themselves, so dePin/rePin stay -1.
// Программный UART на пинах 10 (RX) и 11 (TX).
#define SSerialRx 10
#define SSerialTx 11

Mercury mercury(SSerialRx, SSerialTx);     // дефолты: dePin=-1, rePin=-1 (авто)

void setup() {
    Serial.begin(9600);
    mercury.begin(0, 9600, 150, 2);
    mercury.search();
    mercury.connect(1, "111111");
}

void loop() {
    float* powerP = mercury.powerP();
    Serial.print("Активная мощность: ");
    for (int i = 0; i < 4; i++) {
        Serial.print(powerP[i]);
        Serial.print(" ");
    }
    Serial.println();
    delay(2000);
}
