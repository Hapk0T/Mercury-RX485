#ifndef MERCURY_H
#define MERCURY_H

#include <Arduino.h>
#include <SoftwareSerial.h>

class Mercury {
public:
    Mercury(int rxPin, int txPin, int dePin = -1, int rePin = -1, bool enableDirInfo = false, bool debug = false);
    void begin(byte id = 0, long baud_rate = 9600, int read_delay = 150, int send_delay = 2);
    void search();
    void connect(byte level = 1, const char* password = "111111");
    float* powerP();
    float* powerQ();
    float* powerS();
    float* voltage();
    float* current();
    float* powerFactor();
    float frequency();
    int temperature();
    float* energyTaAr();
    float* energyTaF();
    float* energyT1Ar();
    float* energyT1F();
    float* energyT2Ar();
    float* energyT2F();

    bool dir_a;
    bool dir_r;

private:
    int rxPin, txPin, dePin, rePin;
    bool enableDirInfo, debug;
    byte devId;
    long baud_rate;
    int read_delay, send_delay;
    SoftwareSerial* softSerial;
    HardwareSerial* hardSerial;

    void sendCmd(byte addr, byte* cmd, int s, byte* response);
    unsigned int crc16MODBUS(byte* s, int count);
    void setPins(bool state);
};

#endif
