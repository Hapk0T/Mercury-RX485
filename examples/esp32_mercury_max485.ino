#include <mercury.h>

// ESP32 + MAX485 с ручным управлением направлением (DE/RE).
// SoftwareSerial на ESP32 недоступен, поэтому используется аппаратный UART Serial2.
// DE подключён к GPIO4, RE - к GPIO5 (можно объединить с D/R и задать только dePin).
#define RS485_RX  16   // RO  -> GPIO16 (RX2)
#define RS485_TX  17   // DI  -> GPIO17 (TX2)
#define RS485_DE  4    // DE  -> GPIO4
#define RS485_RE  5    // RE  -> GPIO5

// Пины rxPin/txPin здесь не нужны (порт задаётся через useHardwareSerial),
// но DE/RE обязательны — это «ручное управление направлением».
Mercury mercury(RS485_RX, RS485_TX, RS485_DE, RS485_RE, false, true);

void setup() {
    Serial.begin(9600);
    mercury.useHardwareSerial(&Serial2);   // вызывается ДО begin()
    mercury.begin(0, 9600, 150, 2);
    mercury.search();                      // теперь адрес находится в диапазоне 0..254
    mercury.connect(1, "111111");
}

void loop() {
    float* voltage = mercury.voltage();
    Serial.print("Напряжение: ");
    for (int i = 0; i < 3; i++) {
        Serial.print(voltage[i]);
        Serial.print(" ");
    }
    Serial.println();
    delay(2000);
}
