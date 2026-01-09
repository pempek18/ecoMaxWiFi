#include "communicationWithFurner.hpp"
#include <cstring>
#include <cmath>

CommunicationWithFurner::CommunicationWithFurner(ISerialPort* serialPort)
    : _serial(serialPort)
    , _packetState(WAITING_FOR_HEADER_1)
    , _packetIndex(0)
{
    // Initialize data structure
    std::memset(&_data, 0, sizeof(_data));
    _data.dataValid = false;
    std::memset(_packetBuffer, 0, sizeof(_packetBuffer));
}

bool CommunicationWithFurner::begin() {
    if (_serial == nullptr) {
        return false;
    }

    // Reset state
    _packetState = WAITING_FOR_HEADER_1;
    _packetIndex = 0;

    return true;
}

bool CommunicationWithFurner::update() {
    if (_serial == nullptr) {
        return false;
    }

    // Read available bytes
    while (_serial->available() > 0) {
        uint8_t byte;
        if (!_serial->readByte(byte)) {
            break; // No more bytes available
        }

        switch (_packetState) {
            case WAITING_FOR_HEADER_1:
                if (byte == PACKET_HEADER_1) {
                    _packetState = WAITING_FOR_HEADER_2;
                    _packetBuffer[0] = byte;
                    _packetIndex = 1;
                }
                break;

            case WAITING_FOR_HEADER_2:
                if (byte == PACKET_HEADER_2) {
                    _packetState = WAITING_FOR_HEADER_3;
                    _packetBuffer[1] = byte;
                    _packetIndex = 2;
                } else {
                    // Reset if not matching
                    _packetState = WAITING_FOR_HEADER_1;
                    _packetIndex = 0;
                }
                break;

            case WAITING_FOR_HEADER_3:
                if (byte == PACKET_HEADER_3) {
                    _packetState = READING_PACKET;
                    _packetBuffer[2] = byte;
                    _packetIndex = 3;
                } else {
                    // Reset if not matching
                    _packetState = WAITING_FOR_HEADER_1;
                    _packetIndex = 0;
                }
                break;

            case READING_PACKET:
                if (_packetIndex < PACKET_SIZE) {
                    _packetBuffer[_packetIndex] = byte;
                    _packetIndex++;

                    // Check if packet is complete
                    if (_packetIndex >= PACKET_SIZE) {
                        // Parse the packet
                        parsePacket();
                        
                        // Reset state machine
                        _packetState = WAITING_FOR_HEADER_1;
                        _packetIndex = 0;
                        
                        return true; // New data received
                    }
                } else {
                    // Packet too long, reset
                    _packetState = WAITING_FOR_HEADER_1;
                    _packetIndex = 0;
                }
                break;
        }
    }

    return false; // No new data
}

void CommunicationWithFurner::parsePacket() {
    // Verify packet header
    if (_packetBuffer[0] != PACKET_HEADER_1 ||
        _packetBuffer[1] != PACKET_HEADER_2 ||
        _packetBuffer[2] != PACKET_HEADER_3) {
        _data.dataValid = false;
        return;
    }

    // Extract data according to Python script offsets
    // Temperature return: bytes 86-89 (float, little-endian)
    _data.temperatureReturn = extractFloat(86);

    // Temperature mixer: bytes 90-93 (float, little-endian)
    _data.temperatureMixer = extractFloat(90);

    // Temperature feeder: bytes 106-109 (float, little-endian)
    _data.temperatureFeeder = extractFloat(106);
    
    // If feeder temp is invalid, try alternative offset 71 (from ecomax860p.py)
    if (std::isnan(_data.temperatureFeeder) || std::isinf(_data.temperatureFeeder)) {
        float altTemp = extractFloat(71);
        if (!std::isnan(altTemp) && !std::isinf(altTemp) && altTemp > -50.0f && altTemp < 200.0f) {
            _data.temperatureFeeder = altTemp;
        }
    }

    // Temperature exhaust (spalin): bytes 110-113 (float, little-endian)
    _data.temperatureExhaust = extractFloat(110);

    // Temperature CO: bytes 67-70 (float, little-endian) - from ecomax860p.py
    _data.temperatureCO = extractFloat(67);

    // Temperature CWU: bytes 75-78 (float, little-endian) - from ecomax860p.py
    _data.temperatureCWU = extractFloat(75);

    // Temperature weather (pogodowa): bytes 99-102 (float, little-endian) - from ecomax860p.py
    _data.temperatureWeather = extractFloat(99);

    // Flame percentage: bytes 118-121 (float, little-endian)
    _data.flamePercentage = extractFloat(118);

    // Fuel consumption kg/h: bytes 234-237 (float, little-endian)
    _data.fuelConsumption = extractFloat(234);

    // Fan speed: byte 279 (uint8)
    _data.fanSpeed = extractUint8(279);

    // Power: bytes 250-253 (float, little-endian)
    _data.power = extractFloat(250);

    // Power (byte): byte 196 (uint8) - from ecomax860p.py, more reliable than float
    _data.powerByte = extractUint8(196);

    // Operating status: byte 53 (uint8) - from ecomax860p.py
    _data.operatingStatus = extractUint8(53);

    // Fuel level: byte 168 (uint8) - from ecomax860p.py
    _data.fuelLevel = extractUint8(168);

    // Work time 100%: bytes 294-295 (uint16, little-endian)
    _data.workTime100 = extractUint16(294);

    // Work time 50%: bytes 296-297 (uint16, little-endian)
    _data.workTime50 = extractUint16(296);

    // Work time 33%: bytes 298-299 (uint16, little-endian)
    _data.workTime33 = extractUint16(298);

    // Feeder work time: bytes 300-301 (uint16, little-endian)
    _data.feederWorkTime = extractUint16(300);

    // Burned fuel: bytes 302-305 (float, little-endian)
    _data.burnedFuel = extractFloat(302);

    // Ignitions: bytes 306-307 (uint16, little-endian)
    _data.ignitions = extractUint16(306);

    // Motor lock: bytes 310-311 (uint16, little-endian)
    _data.motorLock = extractUint16(310);

    _data.dataValid = true;
}

float CommunicationWithFurner::extractFloat(size_t offset) const {
    if (offset + 3 >= PACKET_SIZE) {
        return std::numeric_limits<float>::quiet_NaN();
    }

    // Extract 4 bytes as little-endian float
    union {
        uint32_t i;
        float f;
    } converter;

    converter.i = (uint32_t)_packetBuffer[offset] |
                  ((uint32_t)_packetBuffer[offset + 1] << 8) |
                  ((uint32_t)_packetBuffer[offset + 2] << 16) |
                  ((uint32_t)_packetBuffer[offset + 3] << 24);

    // Return the value as-is (including NaN/infinity) - let the caller decide how to handle it
    return converter.f;
}

bool CommunicationWithFurner::isValidFloat(float value) const {
    return !std::isnan(value) && !std::isinf(value);
}

uint16_t CommunicationWithFurner::extractUint16(size_t offset) const {
    if (offset + 1 >= PACKET_SIZE) {
        return 0;
    }

    // Extract 2 bytes as little-endian uint16
    return (uint16_t)_packetBuffer[offset] |
           ((uint16_t)_packetBuffer[offset + 1] << 8);
}

uint8_t CommunicationWithFurner::extractUint8(size_t offset) const {
    if (offset >= PACKET_SIZE) {
        return 0;
    }

    return _packetBuffer[offset];
}

CommunicationWithFurner::FurnaceData CommunicationWithFurner::getData() const {
    return _data;
}

float CommunicationWithFurner::getTemperatureBoiler() const {
    return _data.temperatureBoiler;
}

float CommunicationWithFurner::getTemperatureFeeder() const {
    return _data.temperatureFeeder;
}

float CommunicationWithFurner::getTemperatureReturn() const {
    return _data.temperatureReturn;
}

float CommunicationWithFurner::getTemperatureMixer() const
{
return _data.temperatureMixer;
}

float CommunicationWithFurner::getTemperatureExhaust() const {
    return _data.temperatureExhaust;
}

float CommunicationWithFurner::getTemperatureCO() const {
    return _data.temperatureCO;
}

float CommunicationWithFurner::getTemperatureCWU() const {
    return _data.temperatureCWU;
}

float CommunicationWithFurner::getTemperatureWeather() const {
    return _data.temperatureWeather;
}

float CommunicationWithFurner::getFlamePercentage() const {
    return _data.flamePercentage;
}

float CommunicationWithFurner::getFuelConsumption() const {
    return _data.fuelConsumption;
}

uint8_t CommunicationWithFurner::getFanSpeed() const {
    return _data.fanSpeed;
}

float CommunicationWithFurner::getPower() const {
    return _data.power;
}

uint8_t CommunicationWithFurner::getPowerByte() const {
    return _data.powerByte;
}

uint8_t CommunicationWithFurner::getOperatingStatus() const {
    return _data.operatingStatus;
}

uint8_t CommunicationWithFurner::getFuelLevel() const {
    return _data.fuelLevel;
}

uint16_t CommunicationWithFurner::getWorkTime100() const {
    return _data.workTime100;
}

uint16_t CommunicationWithFurner::getWorkTime50() const {
    return _data.workTime50;
}

uint16_t CommunicationWithFurner::getWorkTime33() const {
    return _data.workTime33;
}

uint16_t CommunicationWithFurner::getFeederWorkTime() const {
    return _data.feederWorkTime;
}

float CommunicationWithFurner::getBurnedFuel() const {
    return _data.burnedFuel;
}

uint16_t CommunicationWithFurner::getIgnitions() const {
    return _data.ignitions;
}

uint16_t CommunicationWithFurner::getMotorLock() const {
    return _data.motorLock;
}

bool CommunicationWithFurner::isDataValid() const {
    return _data.dataValid;
}

const uint8_t* CommunicationWithFurner::getPacketBuffer() const {
    return _packetBuffer;
}

size_t CommunicationWithFurner::getPacketSize() const {
    return PACKET_SIZE;
}