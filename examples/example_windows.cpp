/**
 * @brief Example implementation for Windows using Windows API
 * 
 * This is an example showing how to implement ISerialPort for Windows
 * Compile with: g++ -std=c++11 example_windows.cpp communicationWithFurner.cpp -o furnace_reader.exe
 */

#include "communicationWithFurner.hpp"
#include <windows.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <algorithm>

// Helper function to normalize COM port name
// Windows requires "\\.\COMx" format for ports above COM9
std::string normalizeComPort(const std::string& portName) {
    std::string normalized = portName;
    
    // Convert to uppercase for comparison
    std::string upper = portName;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    
    // If it's already in the correct format, return as is
    if (normalized.find("\\\\.\\") == 0) {
        return normalized;
    }
    
    // If it starts with COM, check if it's COM10 or higher
    if (upper.find("COM") == 0) {
        std::string number = upper.substr(3);
        try {
            int portNum = std::stoi(number);
            // For COM10 and above, use the extended format
            if (portNum >= 10) {
                return "\\\\.\\" + upper;
            }
        } catch (...) {
            // If conversion fails, just return the original
        }
    }
    
    return normalized;
}

// Helper function to get Windows error message
std::string getWindowsErrorMessage(DWORD errorCode) {
    if (errorCode == 0) {
        return "No error";
    }
    
    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&messageBuffer,
        0,
        nullptr
    );
    
    std::string message(messageBuffer, size);
    LocalFree(messageBuffer);
    
    // Remove trailing newlines
    while (!message.empty() && (message.back() == '\n' || message.back() == '\r')) {
        message.pop_back();
    }
    
    return message;
}

class WindowsSerialPort : public ISerialPort {
public:
    WindowsSerialPort(const char* portName, DWORD baudRate = 115200) 
        : _handle(INVALID_HANDLE_VALUE), _portName(portName), _baudRate(baudRate), _lastError(0) {
    }

    ~WindowsSerialPort() override {
        close();
    }

    bool open() {
        // Open serial port
        _handle = CreateFileA(
            _portName.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr
        );

        if (_handle == INVALID_HANDLE_VALUE) {
            _lastError = GetLastError();
            return false;
        }

        // Configure serial port
        DCB dcb;
        std::memset(&dcb, 0, sizeof(DCB));
        dcb.DCBlength = sizeof(DCB);
        
        if (!GetCommState(_handle, &dcb)) {
            _lastError = GetLastError();
            CloseHandle(_handle);
            _handle = INVALID_HANDLE_VALUE;
            return false;
        }

        // Set serial port parameters
        dcb.BaudRate = _baudRate;
        dcb.ByteSize = 8;
        dcb.Parity = NOPARITY;
        dcb.StopBits = ONESTOPBIT;
        dcb.fBinary = TRUE;
        dcb.fParity = FALSE;
        dcb.fOutxCtsFlow = FALSE;
        dcb.fOutxDsrFlow = FALSE;
        dcb.fDtrControl = DTR_CONTROL_DISABLE;
        dcb.fDsrSensitivity = FALSE;
        dcb.fTXContinueOnXoff = FALSE;
        dcb.fOutX = FALSE;
        dcb.fInX = FALSE;
        dcb.fErrorChar = FALSE;
        dcb.fNull = FALSE;
        dcb.fRtsControl = RTS_CONTROL_DISABLE;
        dcb.fAbortOnError = FALSE;

        if (!SetCommState(_handle, &dcb)) {
            _lastError = GetLastError();
            CloseHandle(_handle);
            _handle = INVALID_HANDLE_VALUE;
            return false;
        }

        // Set timeouts
        COMMTIMEOUTS timeouts;
        std::memset(&timeouts, 0, sizeof(COMMTIMEOUTS));
        timeouts.ReadIntervalTimeout = 50;
        timeouts.ReadTotalTimeoutConstant = 50;
        timeouts.ReadTotalTimeoutMultiplier = 10;
        timeouts.WriteTotalTimeoutConstant = 50;
        timeouts.WriteTotalTimeoutMultiplier = 10;
        
        if (!SetCommTimeouts(_handle, &timeouts)) {
            _lastError = GetLastError();
            CloseHandle(_handle);
            _handle = INVALID_HANDLE_VALUE;
            return false;
        }

        return true;
    }
    
    DWORD getLastError() const {
        return _lastError;
    }

    void close() {
        if (_handle != INVALID_HANDLE_VALUE) {
            CloseHandle(_handle);
            _handle = INVALID_HANDLE_VALUE;
        }
    }

    size_t available() const override {
        if (_handle == INVALID_HANDLE_VALUE) {
            return 0;
        }

        DWORD errors;
        COMSTAT status;
        if (ClearCommError(_handle, &errors, &status)) {
            return status.cbInQue;
        }
        return 0;
    }

    bool readByte(uint8_t& byte) override {
        if (_handle == INVALID_HANDLE_VALUE) {
            return false;
        }

        DWORD bytesRead = 0;
        if (ReadFile(_handle, &byte, 1, &bytesRead, nullptr) && bytesRead == 1) {
            return true;
        }
        return false;
    }

    size_t write(const uint8_t* data, size_t length) override {
        if (_handle == INVALID_HANDLE_VALUE) {
            return 0;
        }

        DWORD bytesWritten = 0;
        if (WriteFile(_handle, data, static_cast<DWORD>(length), &bytesWritten, nullptr)) {
            return bytesWritten;
        }
        return 0;
    }

    bool isOpen() const {
        return _handle != INVALID_HANDLE_VALUE;
    }

private:
    HANDLE _handle;
    std::string _portName;
    DWORD _baudRate;
    DWORD _lastError;
};

int main(int argc, char* argv[]) {
    // Get COM port from command line argument or use default
    std::string comPort = "COM3"; // Default COM port
    
    if (argc > 1) {
        std::string arg = argv[1];
        // Check for help
        if (arg == "-h" || arg == "--help" || arg == "/?") {
            std::cout << "Usage: furnace_reader.exe [COM_PORT] [BAUD_RATE]" << std::endl;
            std::cout << std::endl;
            std::cout << "Arguments:" << std::endl;
            std::cout << "  COM_PORT    Serial port name (e.g., COM3, COM4). Default: COM3" << std::endl;
            std::cout << "  BAUD_RATE   Baud rate (default: 115200)" << std::endl;
            std::cout << std::endl;
            std::cout << "Examples:" << std::endl;
            std::cout << "  furnace_reader.exe COM3" << std::endl;
            std::cout << "  furnace_reader.exe COM4 115200" << std::endl;
            return 0;
        }
        comPort = arg;
    }
    
    // Normalize COM port name (required for COM10+)
    std::string normalizedPort = normalizeComPort(comPort);
    
    // Get baud rate from command line argument or use default
    DWORD baudRate = 115200; // Default baud rate
    if (argc > 2) {
        baudRate = static_cast<DWORD>(std::stoul(argv[2]));
    }
    
    std::cout << "Opening serial port: " << normalizedPort << " at " << baudRate << " baud" << std::endl;
    
    WindowsSerialPort serial(normalizedPort.c_str(), baudRate);
    
    if (!serial.open()) {
        DWORD error = serial.getLastError();
        std::cerr << "Failed to open serial port " << normalizedPort << std::endl;
        std::cerr << "Windows error code: " << error << std::endl;
        std::cerr << "Error message: " << getWindowsErrorMessage(error) << std::endl;
        std::cerr << std::endl;
        
        // Provide helpful error messages based on common error codes
        switch (error) {
            case ERROR_FILE_NOT_FOUND:
            case ERROR_INVALID_NAME:
                std::cerr << "Error: Port " << normalizedPort << " does not exist." << std::endl;
                if (normalizedPort != comPort) {
                    std::cerr << "  (Normalized from: " << comPort << ")" << std::endl;
                }
                std::cerr << "Please check the port name. Available ports can be listed with:" << std::endl;
                std::cerr << "  PowerShell: Get-WmiObject Win32_SerialPort | Select-Object DeviceID, Description" << std::endl;
                std::cerr << "  CMD: mode" << std::endl;
                break;
            case ERROR_ACCESS_DENIED:
                std::cerr << "Error: Access denied. The port may be:" << std::endl;
                std::cerr << "  - Already in use by another application" << std::endl;
                std::cerr << "  - Locked by a driver" << std::endl;
                std::cerr << "  - Requires administrator privileges" << std::endl;
                std::cerr << "Try running as administrator or close other applications using the port." << std::endl;
                break;
            case ERROR_SHARING_VIOLATION:
                std::cerr << "Error: Port is already in use by another application." << std::endl;
                std::cerr << "Close other programs using " << normalizedPort << " and try again." << std::endl;
                break;
            case ERROR_INVALID_PARAMETER:
                std::cerr << "Error: Invalid parameter. Check baud rate and port configuration." << std::endl;
                break;
            default:
                std::cerr << "Troubleshooting steps:" << std::endl;
                std::cerr << "  1. Verify the port exists: mode" << std::endl;
                std::cerr << "  2. Check if another application is using the port" << std::endl;
                std::cerr << "  3. Try running as administrator" << std::endl;
                std::cerr << "  4. Restart the computer if the port appears stuck" << std::endl;
                break;
        }
        
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
            }else{
                std::cout << "Data is not valid" << std::endl;
            }
        }else{
            std::cout << "No data received" << std::endl;
        }
        
        Sleep(1000); // Small delay to prevent CPU spinning
    }

    return 0;
}

