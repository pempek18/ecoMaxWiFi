/**
 * @brief Example implementation for Windows using Windows API
 * 
 * This is an example showing how to implement ISerialPort for Windows
 * Compile with: g++ -std=c++11 example_windows.cpp communicationWithFurner.cpp -o furnace_reader.exe
 */

#include "../furnaceReader/communicationWithFurner.hpp"
#include <windows.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <vector>

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

// Helper function to create a hex dump of binary data
std::string hexDump(const uint8_t* data, size_t length, size_t bytesPerLine = 16) {
    std::ostringstream oss;
    oss << std::hex << std::uppercase << std::setfill('0');
    
    for (size_t i = 0; i < length; i += bytesPerLine) {
        // Offset
        oss << std::setw(4) << i << ": ";
        
        // Hex bytes
        for (size_t j = 0; j < bytesPerLine; j++) {
            if (i + j < length) {
                oss << std::setw(2) << static_cast<int>(data[i + j]) << " ";
            } else {
                oss << "   ";
            }
        }
        
        // ASCII representation
        oss << " |";
        for (size_t j = 0; j < bytesPerLine && (i + j) < length; j++) {
            uint8_t byte = data[i + j];
            if (byte >= 32 && byte < 127) {
                oss << static_cast<char>(byte);
            } else {
                oss << ".";
            }
        }
        oss << "|" << std::endl;
    }
    
    return oss.str();
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
        dcb.fDtrControl = DTR_CONTROL_ENABLE;  // Enable DTR (matches .NET SerialPort default)
        dcb.fDsrSensitivity = FALSE;
        dcb.fTXContinueOnXoff = FALSE;
        dcb.fOutX = FALSE;
        dcb.fInX = FALSE;
        dcb.fErrorChar = FALSE;
        dcb.fNull = FALSE;
        dcb.fRtsControl = RTS_CONTROL_ENABLE;  // Enable RTS (matches .NET SerialPort default)
        dcb.fAbortOnError = FALSE;

        if (!SetCommState(_handle, &dcb)) {
            _lastError = GetLastError();
            CloseHandle(_handle);
            _handle = INVALID_HANDLE_VALUE;
            return false;
        }

        // Set timeouts - use more lenient settings for RS485 communication
        // MAXDWORD (0xFFFFFFFF) for ReadIntervalTimeout means wait indefinitely between characters
        COMMTIMEOUTS timeouts;
        std::memset(&timeouts, 0, sizeof(COMMTIMEOUTS));
        timeouts.ReadIntervalTimeout = 0xFFFFFFFF;  // MAXDWORD - wait indefinitely between characters
        timeouts.ReadTotalTimeoutConstant = 1000; // 1 second total timeout (matches PowerShell)
        timeouts.ReadTotalTimeoutMultiplier = 0;  // No per-byte timeout
        timeouts.WriteTotalTimeoutConstant = 50;
        timeouts.WriteTotalTimeoutMultiplier = 10;
        
        if (!SetCommTimeouts(_handle, &timeouts)) {
            _lastError = GetLastError();
            CloseHandle(_handle);
            _handle = INVALID_HANDLE_VALUE;
            return false;
        }
        
        // Enable DTR and RTS - some RS485 adapters require these to be enabled
        // This matches the default behavior of .NET SerialPort
        if (!EscapeCommFunction(_handle, SETDTR)) {
            // Non-fatal, but log if needed
        }
        if (!EscapeCommFunction(_handle, SETRTS)) {
            // Non-fatal, but log if needed
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
    std::cout << "Waiting for packets (header: 0x68 0x69 0x01)..." << std::endl;

    int packetCount = 0;
    int invalidPacketCount = 0;

    while (true) {
        if (furnace.update()) {
            if (furnace.isDataValid()) {
                packetCount++;
                std::cout << "=== Furnace Data (Packet #" << packetCount << ") ===" << std::endl;
                
                // Helper lambda to format float values with validation
                auto formatFloat = [](float value, const char* unit = "") -> std::string {
                    if (std::isnan(value) || std::isinf(value)) {
                        return std::string("N/A (invalid)");
                    }
                    std::ostringstream oss;
                    oss << std::fixed << std::setprecision(2) << value;
                    if (unit[0] != '\0') {
                        oss << " " << unit;
                    }
                    return oss.str();
                };
                
                std::cout << "Return Temp: " << formatFloat(furnace.getTemperatureReturn(), "°C") << std::endl;
                std::cout << "Mixer Temp: " << formatFloat(furnace.getTemperatureMixer(), "°C") << std::endl;
                std::cout << "Feeder Temp: " << formatFloat(furnace.getTemperatureFeeder(), "°C") << std::endl;
                std::cout << "Boiler Temp (Output): " << formatFloat(furnace.getTemperatureBoiler(), "°C") << std::endl;
                std::cout << "Exhaust Temp: " << formatFloat(furnace.getTemperatureExhaust(), "°C") << std::endl;
                std::cout << "Flame: " << formatFloat(furnace.getFlamePercentage(), "%") << std::endl;
                std::cout << "Fuel Consumption: " << formatFloat(furnace.getFuelConsumption(), "kg/h") << std::endl;
                std::cout << "Fan Speed: " << static_cast<int>(furnace.getFanSpeed()) << std::endl;
                std::cout << "Power: " << formatFloat(furnace.getPower(), "%") << std::endl;
                std::cout << "Work Time 100%: " << furnace.getWorkTime100() << " h" << std::endl;
                std::cout << "Work Time 50%: " << furnace.getWorkTime50() << " h" << std::endl;
                std::cout << "Work Time 33%: " << furnace.getWorkTime33() << " h" << std::endl;
                std::cout << "Feeder Work Time: " << furnace.getFeederWorkTime() << " h" << std::endl;
                std::cout << "Burned Fuel: " << formatFloat(furnace.getBurnedFuel(), "kg") << std::endl;
                std::cout << "Ignitions: " << furnace.getIgnitions() << std::endl;
                std::cout << "Motor Lock: " << furnace.getMotorLock() << std::endl;
                
                // Decode additional fields from hex dump for analysis
                const uint8_t* buffer = furnace.getPacketBuffer();
                std::cout << std::endl << "--- Additional Decoded Fields ---" << std::endl;
                
                // Try common temperature sensor offsets
                std::vector<size_t> tempOffsets = {94, 98, 102, 106, 110, 114, 67, 71, 75, 83, 87, 99, 103};
                for (size_t offset : tempOffsets) {
                    if (offset + 3 < furnace.getPacketSize()) {
                        // Check if bytes look like a valid float (not all 0xFF or 0x00)
                        bool looksValid = false;
                        for (size_t i = 0; i < 4; i++) {
                            if (buffer[offset + i] != 0xFF && buffer[offset + i] != 0x00) {
                                looksValid = true;
                                break;
                            }
                        }
                        
                        if (looksValid) {
                            union { uint32_t i; float f; } temp;
                            temp.i = (uint32_t)buffer[offset] | ((uint32_t)buffer[offset + 1] << 8) | 
                                     ((uint32_t)buffer[offset + 2] << 16) | ((uint32_t)buffer[offset + 3] << 24);
                            if (!std::isnan(temp.f) && !std::isinf(temp.f) && 
                                temp.f > -100.0f && temp.f < 500.0f) { // Reasonable temp range
                                std::cout << "Temp @" << offset << "-" << (offset+3) << ": " 
                                          << formatFloat(temp.f, "°C") << std::endl;
                            }
                        }
                    }
                }
                
                // Operating status (byte 53 in ecomax860p, but might be different offset)
                for (size_t offset : {53, 54, 55, 56, 57}) {
                    if (offset < furnace.getPacketSize()) {
                        uint8_t status = buffer[offset];
                        if (status != 0 && status != 0xFF) {
                            std::cout << "Status byte @" << offset << ": 0x" << std::hex 
                                      << static_cast<int>(status) << std::dec;
                            // Common status values
                            if (status == 0) std::cout << " (OFF)";
                            else if (status == 1) std::cout << " (IGNITION)";
                            else if (status == 2) std::cout << " (STABILIZATION)";
                            else if (status == 3) std::cout << " (RUNNING)";
                            else if (status == 5) std::cout << " (EXTINGUISHING)";
                            else if (status == 7) std::cout << " (EXTINGUISHING_ON_DEMAND)";
                            std::cout << std::endl;
                        }
                    }
                }
                
                // Fuel level (byte 168 in ecomax860p)
                for (size_t offset : {168, 169, 170}) {
                    if (offset < furnace.getPacketSize()) {
                        uint8_t fuelLevel = buffer[offset];
                        if (fuelLevel != 0xFF && fuelLevel <= 100) {
                            std::cout << "Fuel Level @" << offset << ": " 
                                      << static_cast<int>(fuelLevel) << "%" << std::endl;
                        }
                    }
                }
                
                // Boiler power as byte (offset 196 in ecomax860p)
                for (size_t offset : {196, 197, 198, 199}) {
                    if (offset < furnace.getPacketSize()) {
                        uint8_t power = buffer[offset];
                        if (power != 0xFF && power <= 100) {
                            std::cout << "Power (byte) @" << offset << ": " 
                                      << static_cast<int>(power) << "%" << std::endl;
                        }
                    }
                }
                
                // Lambda/Oxygen levels (offsets 226-233 in ecomax860p)
                for (size_t offset : {226, 230}) {
                    if (offset + 3 < furnace.getPacketSize()) {
                        union { uint32_t i; float f; } value;
                        value.i = (uint32_t)buffer[offset] | ((uint32_t)buffer[offset + 1] << 8) | 
                                  ((uint32_t)buffer[offset + 2] << 16) | ((uint32_t)buffer[offset + 3] << 24);
                        if (!std::isnan(value.f) && !std::isinf(value.f) && 
                            value.f >= 0.0f && value.f < 100.0f) {
                            std::string label = (offset == 226) ? "Lambda" : "Oxygen";
                            std::cout << label << " @" << offset << "-" << (offset+3) << ": " 
                                      << formatFloat(value.f, "%") << std::endl;
                        }
                    }
                }
                
                std::cout << std::endl;
                std::cout << "Raw packet data (hex dump):" << std::endl;
                std::cout << hexDump(furnace.getPacketBuffer(), furnace.getPacketSize()) << std::endl;
            } else {
                invalidPacketCount++;
                std::cout << "Warning: Received packet but data validation failed (count: " 
                          << invalidPacketCount << ")" << std::endl;
                std::cout << "Raw packet data (hex dump):" << std::endl;
                std::cout << hexDump(furnace.getPacketBuffer(), furnace.getPacketSize()) << std::endl;
            }
        }
        
        Sleep(10); // Small delay to prevent CPU spinning
    }

    return 0;
}

