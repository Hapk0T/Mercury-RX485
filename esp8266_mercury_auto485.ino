#include <mercury.h>

// Пины для программного UART
#define SSerialRx D1
#define SSerialTx D2

// Создание объекта Mercury с использованием программного UART
Mercury mercury(SSerialRx, SSerialTx, -1, -1, true, true);

void setup() {
    Serial.begin(9600);
    mercury.begin(0, 9600, 150, 2);
    mercury.search();
    mercury.connect(1, "111111");
}

void loop() {
    float* powerP = mercury.powerP();
    Serial.print("Active Power: ");
    for (int i = 0; i < 4; i++) {
        Serial.print(powerP[i]);
        Serial.print(" ");
    }
    Serial.println();

    float* voltage = mercury.voltage();
    Serial.print("Voltage: ");
    for (int i = 0; i < 3; i++) {
        Serial.print(voltage[i]);
        Serial.print(" ");
    }
    Serial.println();

    float* current = mercury.current();
    Serial.print("Current: ");
    for (int i = 0; i < 3; i++) {
        Serial.print(current[i]);
        Serial.print(" ");
    }
    Serial.println();

    float frequency = mercury.frequency();
    Serial.print("Frequency: ");
    Serial.println(frequency);

    int temperature = mercury.temperature();
    Serial.print("Temperature: ");
    Serial.println(temperature);

    delay(2000);
}
