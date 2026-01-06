# CommunicationWithFurner - Platform Independent Furnace Communication Library

A C++ library for communicating with ecoMAX/Kostrzewa furnace controllers via RS485. The library is **platform-independent** and works on Windows, Linux, Arduino, ESP32, and any other platform that can provide a serial port interface.

## Features

- **Platform Independent**: Works on Windows, Linux, Arduino, ESP32, and more
- **RS485 Communication**: Reads data packets from furnace controller
- **Complete Data Extraction**: Extracts all furnace parameters:
  - Temperature readings (boiler, feeder, return)
  - Flame percentage
  - Fuel consumption
  - Fan speed
  - Power
  - Work times (100%, 50%, 33%)
  - Burned fuel
  - Ignitions count
  - Motor lock status

## Protocol Details

- **Baud Rate**: 115200
- **Data Format**: 8N1 (8 data bits, no parity, 1 stop bit)
- **Packet Format**: Starts with `0x68 0x69 0x01`, 361 bytes total
- **Data Encoding**: Little-endian for multi-byte values

## Architecture

The library uses an abstract interface pattern:

1. **ISerialPort**: Abstract interface that you implement for your platform
2. **CommunicationWithFurner**: Main class that handles packet parsing and data extraction

## Quick Start

### 1. Implement ISerialPort for Your Platform

You need to implement the `ISerialPort` interface with three methods:

```cpp
class MySerialPort : public ISerialPort {
public:
    size_t available() const override {
        // Return number of bytes available to read
    }
    
    bool readByte(uint8_t& byte) override {
        // Read one byte, return true if successful
    }
    
    size_t write(const uint8_t* data, size_t length) override {
        // Optional: write data (for future use)
    }
};
```

### 2. Use the Library

```cpp
#include "communicationWithFurner.hpp"

// Create your serial port implementation
MySerialPort serial(/* your parameters */);
serial.open(); // Initialize your serial port

// Create furnace communication object
CommunicationWithFurner furnace(&serial);
furnace.begin();

// In your main loop
while (true) {
    if (furnace.update()) {
        if (furnace.isDataValid()) {
            float temp = furnace.getTemperatureBoiler();
            // Use the data...
        }
    }
}
```

## Example Implementations

### Windows Example

See `example_windows.cpp` for a complete Windows implementation using Windows API.

```cpp
WindowsSerialPort serial("COM3", 115200);
serial.open();
CommunicationWithFurner furnace(&serial);
```

### Linux Example

See `example_linux.cpp` for a complete Linux implementation using termios.

```cpp
LinuxSerialPort serial("/dev/ttyUSB0", B115200);
serial.open();
CommunicationWithFurner furnace(&serial);
```

### Arduino/ESP32 Example

See `example_arduino.cpp` for an Arduino/ESP32 implementation.

```cpp
ArduinoSerialPort serial(&Serial2);
CommunicationWithFurner furnace(&serial);
```

## API Reference

### CommunicationWithFurner Class

#### Constructor
```cpp
CommunicationWithFurner(ISerialPort* serialPort)
```

#### Methods

- `bool begin()` - Initialize the communication
- `bool update()` - Read and process incoming data (call regularly)
- `FurnaceData getData()` - Get all data as a structure
- `bool isDataValid()` - Check if current data is valid

#### Individual Getters

- `float getTemperatureBoiler()` - Boiler output temperature (°C)
- `float getTemperatureFeeder()` - Feeder temperature (°C)
- `float getTemperatureReturn()` - Return temperature (°C)
- `float getFlamePercentage()` - Flame percentage (%)
- `float getFuelConsumption()` - Fuel consumption (kg/h)
- `uint8_t getFanSpeed()` - Fan speed
- `float getPower()` - Boiler power
- `uint16_t getWorkTime100()` - Work time at 100% (hours)
- `uint16_t getWorkTime50()` - Work time at 50% (hours)
- `uint16_t getWorkTime33()` - Work time at 33% (hours)
- `uint16_t getFeederWorkTime()` - Feeder work time (hours)
- `float getBurnedFuel()` - Burned fuel (kg)
- `uint16_t getIgnitions()` - Number of ignitions
- `uint16_t getMotorLock()` - Motor lock status

### FurnaceData Structure

```cpp
struct FurnaceData {
    float temperatureBoiler;
    float temperatureFeeder;
    float temperatureReturn;
    float flamePercentage;
    float fuelConsumption;
    uint8_t fanSpeed;
    float power;
    uint16_t workTime100;
    uint16_t workTime50;
    uint16_t workTime33;
    uint16_t feederWorkTime;
    float burnedFuel;
    uint16_t ignitions;
    uint16_t motorLock;
    bool dataValid;
};
```

## Building

### Windows with CMake (Recommended)

#### Using Visual Studio
```bash
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```
The executable will be in `build/bin/Release/furnace_reader.exe`

#### Using MinGW
```bash
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
cmake --build .
```
The executable will be in `build/bin/furnace_reader.exe`

#### Using Build Scripts
- Run `build_windows.bat` for Visual Studio builds
- Run `build_windows_mingw.bat` for MinGW builds

### Windows (Direct Compilation)
```bash
g++ -std=c++11 example_windows.cpp communicationWithFurner.cpp -o furnace_reader.exe
```

### Linux with CMake
```bash
mkdir build
cd build
cmake ..
cmake --build .
```
The executable will be in `build/bin/furnace_reader`

### Linux (Direct Compilation)
```bash
g++ -std=c++11 example_linux.cpp communicationWithFurner.cpp -o furnace_reader
```

### Arduino/ESP32
Add the `.cpp` and `.hpp` files to your Arduino project and include the header.

## Requirements

- C++11 or later
- A serial port implementation for your platform
- RS485 adapter connected to the furnace controller's G2 port

## License

This code is based on the Python script by miszko (miszko@tempra.org, August 2019).

## Notes

- The library only reads data from the furnace (no write functionality yet)
- Packet detection uses a state machine for reliability
- All data extraction matches the original Python script byte offsets
- The library handles little-endian byte order conversion automatically

