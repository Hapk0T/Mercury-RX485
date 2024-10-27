# Mercury Library

Mercury Library — это библиотека для работы с устройствами учёта электроэнергии от компании Инкотекс серии Меркурий, такими как Меркурий 150, 203.2TD, 204, 208, 230, 231, 234, 236, 238, 350. Библиотека поддерживает работу с Arduino, ESP8266, ESP32 и STM32.

## Совместимость

- Arduino
- ESP8266
- ESP32
- STM32

## Возможности и особенности

- Поддержка работы с различными моделями счётчиков Меркурий.
- Возможность работы как с обычными преобразователями MAX485, так и с их версиями/аналогами, которые автоматически переключаются между режимами отправки и приёма.
- Поддержка программного UART (SoftwareSerial) и аппаратного UART (HardwareSerial).
- Возможность включения отладочной информации для удобства разработки.
- Поддержка работы с направлением потока энергии (если включено).

## Описание функций

### Конструктор

```cpp
Mercury(int rxPin, int txPin, int dePin = -1, int rePin = -1, bool enableDirInfo = false, bool debug = false);
```

- `rxPin`: Пин для приёма данных.
- `txPin`: Пин для передачи данных.
- `dePin`: Пин для управления направлением данных (опционально).
- `rePin`: Пин для управления приёмом данных (опционально).
- `enableDirInfo`: Включение информации о направлении потока энергии (опционально).
- `debug`: Включение отладочной информации (опционально).

### Начальная настройка

```cpp
void begin(byte id = 0, long baud_rate = 9600, int read_delay = 150, int send_delay = 2);
```

- `id`: Идентификатор устройства (опционально).
- `baud_rate`: Скорость передачи данных (опционально).
- `read_delay`: Задержка перед чтением данных (опционально).
- `send_delay`: Задержка перед отправкой данных (опционально).

### Поиск устройства

```cpp
void search();
```

### Подключение к устройству

```cpp
void connect(byte level = 1, const char* password = "111111");
```

- `level`: Уровень доступа 1 или 2 (опционально).
- `password`: 6-значный пароль для подключения (опционально).

### Получение данных

#### Активная мощность

```cpp
float* powerP(); // 4 значения, общая по фазам и по каждой фазе
```

#### Реактивная мощность

```cpp
float* powerQ(); // 4 значения, общая по фазам и по каждой фазе
```

#### Полная мощность

```cpp
float* powerS(); // 4 значения, общая по фазам и по каждой фазе
```

#### Напряжение

```cpp
float* voltage(); // 3 значения, 1 фаза, 2 фаза и 3 фаза
```

#### Ток

```cpp
float* current(); // 3 значения, 1 фаза, 2 фаза и 3 фаза
```

#### Коэффициент мощности

```cpp
float* powerFactor(); // 4 значения, общий по фазам и по каждой фазе
```

#### Частота

```cpp
float frequency();
```

#### Температура

```cpp
int temperature();
```

#### Энергия

```cpp
float* energyTaAr(); // 3 значения, 1 фаза, 2 фаза и 3 фаза
float* energyTaF(); // 4 значения, общая по фазам и по каждой фазе
float* energyT1Ar(); // 3 значения, 1 фаза, 2 фаза и 3 фаза
float* energyT1F(); // 4 значения, общая по фазам и по каждой фазе
float* energyT2Ar(); //3 значения, 1 фаза, 2 фаза и 3 фаза
float* energyT2F(); // 4 значения, общая по фазам и по каждой фазе
```

## Тонкости работы с библиотекой

- Для корректной работы с преобразователями MAX485 и их аналогами необходимо правильно настроить пины `dePin` и `rePin`.
- Включение отладочной информации (`debug`) позволяет отслеживать отправляемые и получаемые данные, что упрощает процесс разработки и отладки.
- Включение информации о направлении потока энергии (`enableDirInfo`) позволяет получать дополнительные данные о направлении потока энергии.

## Примеры использования методов

### Пример использования без флагов

```cpp
#include <mercury.h>

// Пины для программного UART
#define SSerialRx D1
#define SSerialTx D2

// Создание объекта Mercury с использованием программного UART
Mercury mercury(SSerialRx, SSerialTx, -1, -1, false, true);

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
```

### Пример использования с флагами

```cpp
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
```

## Заключение

Mercury Library предоставляет удобный интерфейс для работы с устройствами учёта электроэнергии серии Меркурий. Библиотека поддерживает различные платформы и предоставляет широкий набор функций для получения данных с счётчиков.
