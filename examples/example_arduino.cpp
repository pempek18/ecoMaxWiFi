/**
 * @brief Example implementation for Arduino/ESP32
 * 
 * This is an example showing how to implement ISerialPort for Arduino/ESP32
 * Place this in your .ino file or Arduino sketch
 */

#include "communicationWithFurner.hpp"

// For ESP32, you can use Serial2, Serial1, etc.
// For Arduino, you might use Serial or SoftwareSerial
class ArduinoSerialPort : public ISerialPort {
public:
    ArduinoSerialPort(HardwareSerial* serial) : _serial(serial) {}

    size_t available() const override {
        if (_serial == nullptr) return 0;
        return _serial->available();
    }

    bool readByte(uint8_t& byte) override {
        if (_serial == nullptr) return false;
        if (_serial->available() > 0) {
            byte = _serial->read();
            return true;
        }
        return false;
    }

    size_t write(const uint8_t* data, size_t length) override {
        if (_serial == nullptr) return 0;
        return _serial->write(data, length);
    }

private:
    HardwareSerial* _serial;
};

// Example usage for ESP32:
// HardwareSerial Serial2(16, 17); // RX=16, TX=17
// ArduinoSerialPort serial(&Serial2);
// CommunicationWithFurner furnace(&serial);

// Example usage for Arduino:
// ArduinoSerialPort serial(&Serial);
// CommunicationWithFurner furnace(&serial);

