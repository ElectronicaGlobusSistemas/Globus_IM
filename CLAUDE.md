# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an embedded C++ firmware for ESP32-based gaming/casino machines. The system manages:
- **Player Tracking**: RFID-based card reader for player identification and session management
- **Cashless Payments**: AFT (Account Funds Transfer) and EFT (Electronic Funds Transfer) payment processing
- **Gmaster API Integration**: REST API communication for event transmission, counters, and token management
- **TFT Display**: User interface for player interactions
- **SD Card Storage**: Event logging and counter persistence
- **State Machine**: Manages machine states (Player Tracking, Cashless, Operator modes)

## Build Commands

```bash
# Build the project
pio run

# Upload to device (adjust COM port as needed)
pio run --target upload --upload-port COM9

# Monitor serial output
pio device monitor --port COM9 --baud 115200

# Clean build
pio run --target clean

# OTA Upload (when configured)
pio run --upload_port 192.168.5.117 --upload_flag "--auth=FEFE99098F1EAC0361BE7BBCC3342204"
```

## Architecture

### Core State Machine (`Maquina_Estados`)
The firmware operates as a state machine with these primary modes:
- `Player_Tracking` (1): RFID-based player sessions
- `Cashless_AFT` (2): Cashless payments with account transfers
- `Cashless_EFT` (3): Electronic funds transfer
- `Operador_Maquina` (5): Operator/service mode

### Key Components

**`Buffers` & `Buffer_Cashless`**: Manage message buffers between subsystems (RS232, Socket communication)

**`API_Gmaster`**: Handles REST API communication including:
- Token generation and expiration management (5-minute timeout)
- Event transmission to backend
- Counter synchronization
- RTC time synchronization

**`Cashless` & `Fidelizacion` classes**:
- `Cashless`: Manages AFT/EFT transactions, transaction IDs, registration states
- `Fidelizacion`: Handles player tracking sessions and customer information

**`RFID` module**: 
- Card reader polling and event handling
- Session timeout management
- Operator vs customer card distinction

**`SesionPlayerTracking` (New v2.0)**: Refactored session management with:
- State machine: `INACTIVO`, `SESION_ACTIVA`, `PENDIENTE_CIERRE`, `BLOQUEADO`
- Event-driven architecture (bill insertion, card events, inactivity)
- Configurable inactivity timeouts (default 90 seconds)
- Session counter accumulation
- See `docs/SesionPlayerTracking_v2.md` for full documentation

**`Transacciones`**: Transaction processing and accounting

**`Contadores_SAS`**: SAS protocol counter management

**`Variables_Globales`**: Global state flags and variables stored in NVS (Preferences)

### Communication Architecture

- **RS232**: Serial communication with gaming machine (SAS protocol)
- **WiFi/HTTP**: REST API communication via `ESPAsyncWebServer` and `AsyncTCP`
- **SPI**: SD card and RFID reader
- **I2C**: Display and peripheral communication

## Code Patterns

### Global Objects Pattern
Key subsystems are instantiated as global objects in `main.cpp`:
```cpp
ESP32Time RTC;
Buffers Buffer;
Buffer_RX_AFT Buffer_Cashless;
Transsaccion_Cashless Cashless;
Variables_Globales Variables_globales;
API_Gmaster Api_G;
```

### NVS (Non-Volatile Storage)
Uses `Preferences` library for persistent configuration. `Variables_Globales` reads/writes system flags.

### Flag-Based State Management
Extensive use of numeric flag defines (e.g., `Flag_Maquina_En_Juego`, `SD_INSERT`, `Conexion_To_Host`) tracked in `Variables_Globales`.

### Callback Pattern
New `SesionPlayerTracking` uses callbacks for state change notifications.

### Dual Session Systems
The codebase contains both:
- Original `Count_Player_Sesions()` function (still functional)
- New `SPT_Count_Player_Sesions()` wrapper and refactored system
This allows gradual migration without breaking existing functionality.

## Memory Partition

Uses custom partition table `Custom_Memory.csv` for OTA and app/storage allocation.

## Debug Flags

Conditionals defined in `main.cpp` control debug output:
- `Debug_Comunicacion_MQ`
- `Debug_Memoria_SD`
- `Debug_FTP`
- `Debug_Status_SD`
- `Debug_Escritura`
- `Info_SD`

## Dependencies

Key libraries via PlatformIO:
- `ESP32FTPServer`: FTP file server
- `ESP32Time`: Real-time clock
- `ArduinoJson`: JSON parsing
- `PCF8574 library`: I2C I/O expander
- `MFRC522`: RFID reader
- `ESPAsyncWebServer` / `AsyncTCP`: Async web server
- `SdFat`: SD card filesystem

## Important Notes

1. **Timeout Management**: Multiple concurrent timeouts use separate timestamp variables (not `millis()` comparisons)
2. **RTOS Tasks**: Uses FreeRTOS tasks for RFID reading (`Read_RFID`)
3. **Exception Decoder**: Commented out in `platformio.ini` but available for debugging
4. **Bootloader**: Has separate bootloader mode with password `St4rt$B0ot&G1ob5`
5. **OTA Updates**: Supports over-the-air updates when configured
6. **Spanish Comments**: Code contains extensive Spanish language comments
