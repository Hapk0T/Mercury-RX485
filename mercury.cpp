#include "mercury.h"

Mercury::Mercury(int rxPin, int txPin, int dePin, int rePin, bool enableDirInfo, bool debug)
    : rxPin(rxPin), txPin(txPin), dePin(dePin), rePin(rePin), enableDirInfo(enableDirInfo), debug(debug), devId(0), dir_a(false), dir_r(false), connected(false), softSerial(nullptr), hardSerial(nullptr) {}

void Mercury::begin(byte id, long baud_rate, int read_delay, int send_delay) {
    this->devId = id;
    this->baud_rate = baud_rate;
    this->read_delay = read_delay;
    this->send_delay = send_delay;

    if (dePin != -1 || rePin != -1) {
        if (dePin != -1) pinMode(dePin, OUTPUT);
        if (rePin != -1) pinMode(rePin, OUTPUT);
        setPins(false);
    }

    // NOTE: previously the hardware-serial branch was unreachable dead code:
    // softSerial was always nullptr on first begin(), so HardwareSerial (&Serial)
    // was never used and a second begin() leaked the old SoftwareSerial.
    if (hardSerial != nullptr) {
        // caller opted into hardware UART via useHardwareSerial()
        hardSerial->begin(baud_rate);
    } else if (softSerial == nullptr) {
        softSerial = new SoftwareSerial(rxPin, txPin);
        softSerial->begin(baud_rate);
    } else {
        softSerial->end();
        delete softSerial;
        softSerial = new SoftwareSerial(rxPin, txPin);
        softSerial->begin(baud_rate);
    }
}

void Mercury::useHardwareSerial(HardwareSerial* port) {
    // Explicitly select a hardware UART (e.g. Serial / Serial1). When selected,
    // rxPin/txPin are ignored by begin().
    if (port == nullptr) return;
    if (softSerial != nullptr) {
        softSerial->end();
        delete softSerial;
        softSerial = nullptr;
    }
    hardSerial = port;
}

void Mercury::setPins(bool state) {
    // Support modules that wire only DE or only RE: drive the present pin(s).
    if (dePin != -1) digitalWrite(dePin, state);
    if (rePin != -1) digitalWrite(rePin, !state);
}

void Mercury::sendCmd(byte addr, byte* cmd, int s, byte* response) {
    byte fullCmd[s + 3];
    fullCmd[0] = addr;
    memcpy(fullCmd + 1, cmd, s);

    unsigned int crc = crc16MODBUS(fullCmd, s + 1);
    fullCmd[s + 1] = crc & 0xFF;
    fullCmd[s + 2] = (crc >> 8) & 0xFF;

    if (debug) {
        Serial.print("Отправляемые байты: ");
        for (int i = 0; i < s + 3; i++) {
            Serial.print(fullCmd[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }

    // Discard anything left in the RX buffer from previous timed-out / echoed
    // frames. Without this, a single stray byte offsets the whole response by
    // one position and every subsequent CRC check fails ("CRC error" forever).
    while ((softSerial ? softSerial->available() : hardSerial->available()) > 0) {
        (void)(softSerial ? softSerial->read() : hardSerial->read());
    }

    setPins(true);
    if (softSerial) {
        softSerial->write(fullCmd, s + 3);
        softSerial->flush();
    } else {
        hardSerial->write(fullCmd, s + 3);
        hardSerial->flush();
    }
    // Give the transceiver time to switch from TX to RX before reading.
    delay(2);
    setPins(false);

    // Wait until the first byte arrives (bounded by read_delay), then drain the
    // rest with a short inter-byte timeout instead of a fixed blind delay.
    unsigned long start = millis();
    int available = softSerial ? softSerial->available() : hardSerial->available();
    while (available == 0 && (millis() - start) < (unsigned long)read_delay) {
        delay(1);
        available = softSerial ? softSerial->available() : hardSerial->available();
    }

    int responseLength = 0;
    unsigned long lastByteAt = millis();
    available = softSerial ? softSerial->available() : hardSerial->available();
    while (available > 0) {
        byte received = softSerial ? softSerial->read() : hardSerial->read();
        response[responseLength++] = received;
        lastByteAt = millis();
        // wait for possible continuation bytes of the same frame
        while (available == 0 && (millis() - lastByteAt) < 10) {
            delay(1);
            available = softSerial ? softSerial->available() : hardSerial->available();
        }
        available = softSerial ? softSerial->available() : hardSerial->available();
    }

    if (debug) {
        Serial.print("Полученные байты: ");
        for (int i = 0; i < responseLength; i++) {
            Serial.print(response[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }

    // Guard against empty/partial responses: previously response[-2]/response[-1]
    // were read on an untouched buffer (undefined behaviour -> random "found"
    // addresses in search(), garbage values, crashes on some cores).
    if (responseLength < 3) {
        if (debug) {
            Serial.println("Нет ответа (таймаут)");
        }
        response[0] = 0;
        return;
    }

    unsigned int receivedCrc = (response[responseLength - 2] | (response[responseLength - 1] << 8));
    unsigned int calculatedCrc = crc16MODBUS(response, responseLength - 2);
    if (receivedCrc != calculatedCrc) {
        if (debug) {
            Serial.println("CRC error");
        }
        // Keep raw bytes but mark the frame as invalid so callers don't silently
        // parse shifted/garbage data as if it were valid.
        response[0] = 0;
    } else {
        responseLength -= 2;
        response[responseLength] = 0;
    }
}

unsigned int Mercury::crc16MODBUS(byte* s, int count) {
    unsigned int crcTable[] = {
        0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
        0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
        0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
        0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
        0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
        0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
        0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
        0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
        0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
        0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
        0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
        0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
        0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
        0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
        0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
        0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
        0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
        0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
        0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
        0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
        0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
        0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
        0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
        0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
        0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
        0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
        0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
        0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
        0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
        0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
        0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
        0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
    };

    unsigned int crc = 0xFFFF;

    for (int i = 0; i < count; i++) {
        crc = ((crc >> 8) ^ crcTable[(crc ^ s[i]) & 0xFF]);
    }

    return crc;
}

void Mercury::search() {
    byte response[8];
    bool found = false;
    // Scan the full address range supported by Меркурий: 0..254.
    // The old code scanned only 1..127, so meters with addresses >=128
    // (very common: 170/0xAA, 250, 254...) or the factory default address 0
    // could NEVER be found — after a failed search every request kept going
    // to the previous devId and the meter stayed silent ("не видит счётчик").
    for (int addr = 0; addr <= 254; addr++) {
        byte cmd[] = { 0x00 };
        sendCmd(addr, cmd, sizeof(cmd), response);

        if (response[0] == addr && response[1] == 0x00) {
            devId = addr;
            found = true;
            if (debug) {
                Serial.print("Найден адрес: ");
                Serial.println(devId, DEC);
                Serial.print("Ответ: ");
                Serial.print(response[1], HEX);
                Serial.println();
            }
            break;
        } else {
            if (debug) {
                Serial.print("Адрес " + String(addr) + " ответ ");
                Serial.print(response[1], HEX);
                Serial.println();
            }
        }
    }
    if (!found && debug) {
        Serial.println("Счётчик не найден в диапазоне 0..254");
    }
    delay(read_delay);
}

void Mercury::connect(byte level, const char* password) {
    byte response[8];
    byte cmd[] = { 0x01, level, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 };
    for (int i = 0; i < 6; i++) {
        cmd[2 + i] = password[i] - '0';
    }
    sendCmd(devId, cmd, sizeof(cmd), response);

    // sendCmd zeroes response[0] on timeout/CRC failure, so a valid open
    // frame is recognised by the echoed address, not just response[1].
    if (response[0] == devId && response[1] == 0x00) {
        connected = true;
        if (debug) {
            Serial.println("Соединение установлено");
        }
    } else {
        connected = false;
        if (debug) {
            Serial.println("Ошибка соединения");
        }
    }
    delay(send_delay);
}

float* Mercury::powerP() {
    static float power[4];
    byte response[19];
    byte cmd[] = { 0x08, 0x16, 0x00 };
    sendCmd(devId, cmd, sizeof(cmd), response);

    if (enableDirInfo) {
        dir_a = (response[3] & 0x80) >> 7;
        dir_r = (response[3] & 0x40) >> 6;
    }

    for (int i = 0; i < 4; i++) {
        unsigned long value = (response[3 + i * 4] & 0x3F) | (response[2 + i * 4] << 8) | (response[1 + i * 4] << 16);
        power[i] = value / 100.0;
    }
    return power;
}

float* Mercury::powerQ() {
    static float power[4];
    byte response[19];
    byte cmd[] = { 0x08, 0x16, 0x04 };
    sendCmd(devId, cmd, sizeof(cmd), response);

    if (enableDirInfo) {
        dir_a = (response[3] & 0x80) >> 7;
        dir_r = (response[3] & 0x40) >> 6;
    }

    for (int i = 0; i < 4; i++) {
        unsigned long value = (response[3 + i * 4] & 0x3F) | (response[2 + i * 4] << 8) | (response[1 + i * 4] << 16);
        power[i] = value / 100.0;
    }
    return power;
}

float* Mercury::powerS() {
    static float power[4];
    byte response[19];
    byte cmd[] = { 0x08, 0x16, 0x08 };
    sendCmd(devId, cmd, sizeof(cmd), response);

    if (enableDirInfo) {
        dir_a = (response[3] & 0x80) >> 7;
        dir_r = (response[3] & 0x40) >> 6;
    }

    for (int i = 0; i < 4; i++) {
        unsigned long value = (response[3 + i * 4] & 0x3F) | (response[2 + i * 4] << 8) | (response[1 + i * 4] << 16);
        power[i] = value / 100.0;
    }
    return power;
}

float* Mercury::powerFactor() {
    static float powerFactor[4];
    byte response[19];
    byte cmd[] = { 0x08, 0x16, 0x30 };
    sendCmd(devId, cmd, sizeof(cmd), response);

    if (enableDirInfo) {
        dir_a = (response[3] & 0x80) >> 7;
        dir_r = (response[3] & 0x40) >> 6;
    }

    for (int i = 0; i < 4; i++) {
        unsigned long value = (response[3 + i * 4] & 0x3F) | (response[2 + i * 4] << 8) | (response[1 + i * 4] << 16);
        powerFactor[i] = value / 100.0;
    }
    return powerFactor;
}

float* Mercury::voltage() {
    static float voltage[3];
    byte response[16];   // was [4]: a real reply frame is longer and overflowed the stack buffer
    byte cmd[] = { 0x08, 0x16, 0x11 };

    for (int i = 0; i < 3; i++) {
        cmd[2] = 0x11 + i;
        memset(response, 0, sizeof(response));
        sendCmd(devId, cmd, sizeof(cmd), response);
        if (response[0] == 0) { voltage[i] = 0; continue; }  // timeout/CRC error
        unsigned long value = (response[3] << 8) | response[2];
        voltage[i] = value * 0.01;
        delay(send_delay);
    }
    return voltage;
}

float* Mercury::current() {
    static float current[3];
    byte response[13];
    byte cmd[] = { 0x08, 0x16, 0x21 };
    sendCmd(devId, cmd, sizeof(cmd), response);

    for (int i = 0; i < 3; i++) {
        unsigned long value = (response[3 + i * 4] & 0x3F) | (response[2 + i * 4] << 8) | (response[1 + i * 4] << 16);
        current[i] = value / 100.0;
    }
    return current;
}

float Mercury::frequency() {
    byte response[16];
    memset(response, 0, sizeof(response));
    byte cmd[] = { 0x08, 0x11, 0x40 };
    sendCmd(devId, cmd, sizeof(cmd), response);
    if (response[0] == 0) return 0;  // timeout/CRC error
    unsigned long value = (response[3] & 0x3F) << 8 | response[2];
    return value / 100.0;
}

int Mercury::temperature() {
    byte response[16];
    memset(response, 0, sizeof(response));
    byte cmd[] = { 0x08, 0x11, 0x70 };
    sendCmd(devId, cmd, sizeof(cmd), response);
    if (response[0] == 0) return 0;  // timeout/CRC error
    return (signed char)response[2];  // temperature is signed in Меркурий
}

float* Mercury::energyTaAr() {
    static float energy[4];
    byte response[19];
    byte cmd[] = { 0x08, 0x16, 0x40 };
    sendCmd(devId, cmd, sizeof(cmd), response);

    if (enableDirInfo) {
        dir_a = (response[3] & 0x80) >> 7;
        dir_r = (response[3] & 0x40) >> 6;
    }

    for (int i = 0; i < 4; i++) {
        unsigned long value = (response[3 + i * 4] & 0x3F) | (response[2 + i * 4] << 8) | (response[1 + i * 4] << 16);
        energy[i] = value / 100.0;
    }
    return energy;
}

float* Mercury::energyTaF() {
    static float energy[4];
    byte response[19];
    byte cmd[] = { 0x08, 0x16, 0x44 };
    sendCmd(devId, cmd, sizeof(cmd), response);

    if (enableDirInfo) {
        dir_a = (response[3] & 0x80) >> 7;
        dir_r = (response[3] & 0x40) >> 6;
    }

    for (int i = 0; i < 4; i++) {
        unsigned long value = (response[3 + i * 4] & 0x3F) | (response[2 + i * 4] << 8) | (response[1 + i * 4] << 16);
        energy[i] = value / 100.0;
    }
    return energy;
}

float* Mercury::energyT1Ar() {
    static float energy[4];
    byte response[19];
    byte cmd[] = { 0x08, 0x16, 0x48 };
    sendCmd(devId, cmd, sizeof(cmd), response);

    if (enableDirInfo) {
        dir_a = (response[3] & 0x80) >> 7;
        dir_r = (response[3] & 0x40) >> 6;
    }

    for (int i = 0; i < 4; i++) {
        unsigned long value = (response[3 + i * 4] & 0x3F) | (response[2 + i * 4] << 8) | (response[1 + i * 4] << 16);
        energy[i] = value / 100.0;
    }
    return energy;
}

float* Mercury::energyT1F() {
    static float energy[4];
    byte response[19];
    byte cmd[] = { 0x08, 0x16, 0x4C };
    sendCmd(devId, cmd, sizeof(cmd), response);

    if (enableDirInfo) {
        dir_a = (response[3] & 0x80) >> 7;
        dir_r = (response[3] & 0x40) >> 6;
    }

    for (int i = 0; i < 4; i++) {
        unsigned long value = (response[3 + i * 4] & 0x3F) | (response[2 + i * 4] << 8) | (response[1 + i * 4] << 16);
        energy[i] = value / 100.0;
    }
    return energy;
}

float* Mercury::energyT2Ar() {
    static float energy[4];
    byte response[19];
    byte cmd[] = { 0x08, 0x16, 0x50 };
    sendCmd(devId, cmd, sizeof(cmd), response);

    if (enableDirInfo) {
        dir_a = (response[3] & 0x80) >> 7;
        dir_r = (response[3] & 0x40) >> 6;
    }

    for (int i = 0; i < 4; i++) {
        unsigned long value = (response[3 + i * 4] & 0x3F) | (response[2 + i * 4] << 8) | (response[1 + i * 4] << 16);
        energy[i] = value / 100.0;
    }
    return energy;
}

float* Mercury::energyT2F() {
    static float energy[4];
    byte response[19];
    byte cmd[] = { 0x08, 0x16, 0x54 };
    sendCmd(devId, cmd, sizeof(cmd), response);

    if (enableDirInfo) {
        dir_a = (response[3] & 0x80) >> 7;
        dir_r = (response[3] & 0x40) >> 6;
    }

    for (int i = 0; i < 4; i++) {
        unsigned long value = (response[3 + i * 4] & 0x3F) | (response[2 + i * 4] << 8) | (response[1 + i * 4] << 16);
        energy[i] = value / 100.0;
    }
    return energy;
}
