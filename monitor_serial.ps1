# PowerShell Serial Port Monitor
# Usage: .\monitor_serial.ps1 -Port COM3 -BaudRate 115200

param(
    [Parameter(Mandatory=$true)]
    [string]$Port,
    
    [Parameter(Mandatory=$false)]
    [int]$BaudRate = 115200,
    
    [Parameter(Mandatory=$false)]
    [int]$DataBits = 8,
    
    [Parameter(Mandatory=$false)]
    [ValidateSet("None", "Odd", "Even", "Mark", "Space")]
    [string]$Parity = "None",
    
    [Parameter(Mandatory=$false)]
    [ValidateSet("None", "One", "Two", "OnePointFive")]
    [string]$StopBits = "One",
    
    [Parameter(Mandatory=$false)]
    [switch]$ShowHex,
    
    [Parameter(Mandatory=$false)]
    [switch]$ShowAscii
)

# SerialPort is available in System.dll (loaded by default in Windows PowerShell)
# For PowerShell Core, we may need to load it explicitly
try {
    $null = [System.IO.Ports.SerialPort]
} catch {
    # Try to load System.IO.Ports for PowerShell Core
    try {
        Add-Type -AssemblyName System.IO.Ports
    } catch {
        Write-Host "Error: Cannot access System.IO.Ports.SerialPort" -ForegroundColor Red
        Write-Host "This script requires Windows PowerShell or PowerShell with .NET support." -ForegroundColor Red
        exit 1
    }
}

# Normalize COM port name - .NET SerialPort doesn't need \\.\ prefix
# Just ensure it's in the format COMx (case insensitive, works with COM1-COM256)
$Port = $Port -replace '^\\\\.\\', ''  # Remove \\.\ if present
$Port = $Port.ToUpper()  # Convert to uppercase
if (-not $Port -match '^COM\d+$') {
    Write-Host "Error: Invalid port name format. Use COMx (e.g., COM3, COM15)" -ForegroundColor Red
    exit 1
}

Write-Host "Opening serial port: $Port at $BaudRate baud" -ForegroundColor Green
Write-Host "Press Ctrl+C to stop monitoring" -ForegroundColor Yellow
Write-Host ""

try {
    # Create and configure serial port
    $serialPort = New-Object System.IO.Ports.SerialPort($Port, $BaudRate, $Parity, $DataBits, $StopBits)
    $serialPort.ReadTimeout = 1000
    $serialPort.WriteTimeout = 1000
    $serialPort.Open()
    
    if (-not $serialPort.IsOpen) {
        Write-Host "Failed to open port $Port" -ForegroundColor Red
        exit 1
    }
    
    Write-Host "Port opened successfully. Monitoring data..." -ForegroundColor Green
    Write-Host ("=" * 60) -ForegroundColor Cyan
    
    $buffer = New-Object System.Byte[] 1024
    $byteCount = 0
    $startTime = Get-Date
    
    while ($true) {
        try {
            if ($serialPort.BytesToRead -gt 0) {
                $bytesRead = $serialPort.Read($buffer, 0, $buffer.Length)
                
                if ($bytesRead -gt 0) {
                    $byteCount += $bytesRead
                    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss.fff"
                    
                    # Display data
                    if ($ShowHex) {
                        $hexString = ($buffer[0..($bytesRead-1)] | ForEach-Object { "{0:X2}" -f $_ }) -join " "
                        Write-Host "[$timestamp] ($bytesRead bytes) HEX: $hexString" -ForegroundColor Cyan
                    }
                    
                    if ($ShowAscii) {
                        $asciiString = [System.Text.Encoding]::ASCII.GetString($buffer, 0, $bytesRead)
                        $asciiString = $asciiString -replace "`r", "\r" -replace "`n", "\n" -replace "`t", "\t"
                        Write-Host "[$timestamp] ($bytesRead bytes) ASCII: $asciiString" -ForegroundColor Yellow
                    }
                    
                    if (-not $ShowHex -and -not $ShowAscii) {
                        # Default: show both hex and ASCII
                        $hexString = ($buffer[0..($bytesRead-1)] | ForEach-Object { "{0:X2}" -f $_ }) -join " "
                        $asciiString = [System.Text.Encoding]::ASCII.GetString($buffer, 0, $bytesRead)
                        $asciiString = $asciiString -replace '[^\x20-\x7E]', '.'
                        
                        Write-Host "[$timestamp] ($bytesRead bytes)" -ForegroundColor White
                        Write-Host "  HEX:   $hexString" -ForegroundColor Cyan
                        Write-Host "  ASCII: $asciiString" -ForegroundColor Yellow
                    }
                    
                    Write-Host ""
                }
            } else {
                Start-Sleep -Milliseconds 10
            }
        } catch {
            if ($_.Exception.Message -notmatch "timeout") {
                Write-Host "Error reading: $($_.Exception.Message)" -ForegroundColor Red
            }
        }
    }
} catch {
    Write-Host "Error: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
} finally {
    if ($serialPort -and $serialPort.IsOpen) {
        $serialPort.Close()
        Write-Host "`nPort closed. Total bytes received: $byteCount" -ForegroundColor Green
    }
}

