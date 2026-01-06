/**
 * @brief Example implementation for Linux using termios
 * 
 * This is an example showing how to implement ISerialPort for Linux
 * Compile with: g++ -std=c++11 example_linux.cpp communicationWithFurner.cpp -o furnace_reader
 */

#include "communicationWithFurner.hpp"
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <iostream>
#include <string>

class LinuxSerialPort : public ISerialPort {
public:
    LinuxSerialPort(const std::string& device, speed_t baudRate = B115200)
        : _fd(-1), _device(device), _baudRate(baudRate) {
    }

    ~LinuxSerialPort() override {
        close();
    }

    bool open() {
        _fd = ::open(_device.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
        if (_fd < 0) {
            return false;
        }

        // Configure serial port
        struct termios options;
        if (tcgetattr(_fd, &options) != 0) {
            ::close(_fd);
            _fd = -1;
            return false;
        }

        // Set baud rate
        cfsetispeed(&options, _baudRate);
        cfsetospeed(&options, _baudRate);

        // 8N1 configuration
        options.c_cflag &= ~PARENB;   // No parity
        options.c_cflag &= ~CSTOPB;   // 1 stop bit
        options.c_cflag &= ~CSIZE;    // Clear size bits
        options.c_cflag |= CS8;       // 8 data bits
        options.c_cflag &= ~CRTSCTS;   // No hardware flow control
        options.c_cflag |= CREAD | CLOCAL; // Enable receiver, ignore modem control lines

        // Raw input mode
        options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

        // Disable software flow control
        options.c_iflag &= ~(IXON | IXOFF | IXANY);

        // Raw output mode
        options.c_oflag &= ~OPOST;

        // Set timeouts
        options.c_cc[VMIN] = 0;   // Non-blocking read
        options.c_cc[VTIME] = 1;  // 0.1 second timeout

        if (tcsetattr(_fd, TCSANOW, &options) != 0) {
            ::close(_fd);
            _fd = -1;
            return false;
        }

        return true;
    }

    void close() {
        if (_fd >= 0) {
            ::close(_fd);
            _fd = -1;
        }
    }

    size_t available() const override {
        if (_fd < 0) {
            return 0;
        }

        int bytes = 0;
        if (ioctl(_fd, FIONREAD, &bytes) == 0) {
            return static_cast<size_t>(bytes);
        }
        return 0;
    }

    bool readByte(uint8_t& byte) override {
        if (_fd < 0) {
            return false;
        }

        ssize_t result = ::read(_fd, &byte, 1);
        return (result == 1);
    }

    size_t write(const uint8_t* data, size_t length) override {
        if (_fd < 0) {
            return 0;
        }

        ssize_t result = ::write(_fd, data, length);
        return (result >= 0) ? static_cast<size_t>(result) : 0;
    }

    bool isOpen() const {
        return _fd >= 0;
    }

private:
    int _fd;
    std::string _device;
    speed_t _baudRate;
};

int main() {
    // Example usage on Linux
    // Replace "/dev/ttyUSB0" with your actual device
    LinuxSerialPort serial("/dev/ttyUSB0", B115200);
    
    if (!serial.open()) {
        std::cerr << "Failed to open serial port /dev/ttyUSB0" << std::endl;
        return 1;
    }

    CommunicationWithFurner furnace(&serial);
    furnace.begin();

    std::cout << "Reading furnace data... Press Ctrl+C to exit" << std::endl;

    while (true) {
        if (furnace.update()) {
            if (furnace.isDataValid()) {
                std::cout << "=== Furnace Data ===" << std::endl;
                std::cout << "Boiler Temp: " << furnace.getTemperatureBoiler() << " °C" << std::endl;
                std::cout << "Feeder Temp: " << furnace.getTemperatureFeeder() << " °C" << std::endl;
                std::cout << "Return Temp: " << furnace.getTemperatureReturn() << " °C" << std::endl;
                std::cout << "Flame: " << furnace.getFlamePercentage() << " %" << std::endl;
                std::cout << "Fuel Consumption: " << furnace.getFuelConsumption() << " kg/h" << std::endl;
                std::cout << "Fan Speed: " << static_cast<int>(furnace.getFanSpeed()) << std::endl;
                std::cout << "Power: " << furnace.getPower() << std::endl;
                std::cout << "Work Time 100%: " << furnace.getWorkTime100() << " h" << std::endl;
                std::cout << "Work Time 50%: " << furnace.getWorkTime50() << " h" << std::endl;
                std::cout << "Work Time 33%: " << furnace.getWorkTime33() << " h" << std::endl;
                std::cout << "Feeder Work Time: " << furnace.getFeederWorkTime() << " h" << std::endl;
                std::cout << "Burned Fuel: " << furnace.getBurnedFuel() << " kg" << std::endl;
                std::cout << "Ignitions: " << furnace.getIgnitions() << std::endl;
                std::cout << "Motor Lock: " << furnace.getMotorLock() << std::endl;
                std::cout << std::endl;
            }
        }
        
        usleep(10000); // 10ms delay
    }

    return 0;
}

