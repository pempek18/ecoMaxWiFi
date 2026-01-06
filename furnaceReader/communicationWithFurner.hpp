#ifndef COMMUNICATION_WITH_FURNER_HPP
#define COMMUNICATION_WITH_FURNER_HPP

#include <cstdint>
#include <cstddef>
#include <cstring>

/**
 * @brief Abstract serial port interface for platform-independent communication
 * 
 * Implement this interface for your specific platform (Windows, Linux, Arduino, etc.)
 */
class ISerialPort {
public:
    virtual ~ISerialPort() = default;

    /**
     * @brief Check if data is available to read
     * @return Number of bytes available, or 0 if none
     */
    virtual size_t available() const = 0;

    /**
     * @brief Read a single byte from the serial port
     * @param byte Reference to store the read byte
     * @return true if a byte was successfully read, false otherwise
     */
    virtual bool readByte(uint8_t& byte) = 0;

    /**
     * @brief Write bytes to the serial port (optional, for future use)
     * @param data Pointer to data buffer
     * @param length Number of bytes to write
     * @return Number of bytes actually written
     */
    virtual size_t write(const uint8_t* data, size_t length) {
        (void)data;
        (void)length;
        return 0; // Default: not implemented
    }
};

/**
 * @brief Class for communicating with ecoMAX/Kostrzewa furnace via RS485
 * 
 * This class reads data packets from the furnace controller via RS485.
 * Packets start with 0x68 0x69 0x01 and are 361 bytes long.
 * Communication parameters: 115200 baud, 8N1
 * 
 * Platform-independent: works with any serial port implementation
 */
class CommunicationWithFurner {
public:
    /**
     * @brief Structure to hold all furnace data
     */
    struct FurnaceData {
        float temperatureBoiler;      // Temperature output from boiler (°C)
        float temperatureFeeder;      // Temperature feeder (°C)
        float temperatureReturn;      // Temperature return (°C)
        float flamePercentage;        // Flame percentage (%)
        float fuelConsumption;        // Fuel consumption (kg/h)
        uint8_t fanSpeed;             // Fan speed
        float power;                   // Boiler power
        uint16_t workTime100;         // Work time at 100% (hours)
        uint16_t workTime50;          // Work time at 50% (hours)
        uint16_t workTime33;          // Work time at 33% (hours)
        uint16_t feederWorkTime;      // Feeder work time (hours)
        float burnedFuel;             // Burned fuel (kg)
        uint16_t ignitions;           // Number of ignitions
        uint16_t motorLock;           // Motor lock status
        bool dataValid;               // True if data was successfully parsed
    };

    /**
     * @brief Constructor
     * @param serialPort Pointer to serial port interface implementation
     *                   The caller is responsible for managing the lifetime of this object
     */
    explicit CommunicationWithFurner(ISerialPort* serialPort);

    /**
     * @brief Initialize the communication
     * @return true if initialization successful
     */
    bool begin();

    /**
     * @brief Update data by reading from serial port
     * Call this regularly in your main loop
     * @return true if new data was received and parsed
     */
    bool update();

    /**
     * @brief Get the latest furnace data
     * @return FurnaceData structure with all values
     */
    FurnaceData getData() const;

    /**
     * @brief Get temperature boiler (output)
     * @return Temperature in °C
     */
    float getTemperatureBoiler() const;

    /**
     * @brief Get temperature feeder
     * @return Temperature in °C
     */
    float getTemperatureFeeder() const;

    /**
     * @brief Get temperature return
     * @return Temperature in °C
     */
    float getTemperatureReturn() const;

    /**
     * @brief Get flame percentage
     * @return Flame percentage (0-100%)
     */
    float getFlamePercentage() const;

    /**
     * @brief Get fuel consumption
     * @return Fuel consumption in kg/h
     */
    float getFuelConsumption() const;

    /**
     * @brief Get fan speed
     * @return Fan speed value
     */
    uint8_t getFanSpeed() const;

    /**
     * @brief Get boiler power
     * @return Power value
     */
    float getPower() const;

    /**
     * @brief Get work time at 100%
     * @return Work time in hours
     */
    uint16_t getWorkTime100() const;

    /**
     * @brief Get work time at 50%
     * @return Work time in hours
     */
    uint16_t getWorkTime50() const;

    /**
     * @brief Get work time at 33%
     * @return Work time in hours
     */
    uint16_t getWorkTime33() const;

    /**
     * @brief Get feeder work time
     * @return Work time in hours
     */
    uint16_t getFeederWorkTime() const;

    /**
     * @brief Get burned fuel
     * @return Burned fuel in kg
     */
    float getBurnedFuel() const;

    /**
     * @brief Get number of ignitions
     * @return Number of ignitions
     */
    uint16_t getIgnitions() const;

    /**
     * @brief Get motor lock status
     * @return Motor lock value
     */
    uint16_t getMotorLock() const;

    /**
     * @brief Check if data is valid
     * @return true if data was successfully parsed
     */
    bool isDataValid() const;

private:
    static constexpr uint8_t PACKET_HEADER_1 = 0x68;
    static constexpr uint8_t PACKET_HEADER_2 = 0x69;
    static constexpr uint8_t PACKET_HEADER_3 = 0x01;
    static constexpr size_t PACKET_SIZE = 361;

    ISerialPort* _serial;

    uint8_t _packetBuffer[PACKET_SIZE];
    FurnaceData _data;
    
    // State machine for packet detection
    enum PacketState {
        WAITING_FOR_HEADER_1,
        WAITING_FOR_HEADER_2,
        WAITING_FOR_HEADER_3,
        READING_PACKET
    };
    PacketState _packetState;
    size_t _packetIndex;

    /**
     * @brief Extract float from packet at given offset (little-endian)
     * @param offset Byte offset in packet
     * @return Float value
     */
    float extractFloat(size_t offset) const;

    /**
     * @brief Extract uint16 from packet at given offset (little-endian)
     * @param offset Byte offset in packet
     * @return uint16 value
     */
    uint16_t extractUint16(size_t offset) const;

    /**
     * @brief Extract uint8 from packet at given offset
     * @param offset Byte offset in packet
     * @return uint8 value
     */
    uint8_t extractUint8(size_t offset) const;

    /**
     * @brief Parse the received packet and update data structure
     */
    void parsePacket();
};

#endif // COMMUNICATION_WITH_FURNER_HPP
