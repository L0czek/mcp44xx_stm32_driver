# MCP44XX Digital Potentiometer Driver

I2C driver for the Microchip MCP444x/446x series of digital potentiometers with 7-bit or 8-bit wiper resolution for STM32 microcontrollers.

[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![C++ Standard](https://img.shields.io/badge/C++17-blue.svg)](https://en.cppreference.com/w/cpp/17)

## Features

- **I2C Interface**: Standard mode (100kHz) and fast mode (400kHz) support
- **Multiple Wipers**: Supports all 4 wipers (W0-W3) independently
- **Resolution Options**: 7-bit (128 taps) or 8-bit (256 taps) selectable
- **Volatile & Non-Volatile**: Store settings in SRAM or EEPROM
- **Increment/Decrement**: Step wiper up or down by 1
- **EEPROM Management**: Automatic wait for EEPROM write completion
- **Status Monitoring**: Write lock and EEPROM busy status
- **TCON Registers**: Tap control register access for advanced configuration
- **EEPROM Slots**: 5 programmable EEPROM slots for configuration storage

## Integration as Submodule

Add this driver as a Git submodule to your project:

```bash
git submodule add https://github.com/yourorg/mcp44xx_stm32_driver.git vendor/mcp44xx
```

In your project's `CMakeLists.txt`:

```cmake
# Add the driver as a subdirectory
add_subdirectory(vendor/mcp44xx/library)

# Link the library to your target
target_link_libraries(your_target PRIVATE mcp44xx)
```

## Requirements

- C++17 compiler (for `std::expected`)
- STM32 HAL library (must be defined in parent project as `stm32_hal` target)
- I2C peripheral support

## Quick Start

```cpp
#include "mcp44xx.hpp"

// Create 7-bit I2C address (A1=0, A0=0)
uint8_t address = mcp44xx::make_address(0, 0);  // 0x2C

// Initialize driver with I2C handle
mcp44xx::DigitalPotentiometer pot(&hi2c, address, mcp44xx::Resolution::Bits8);

// Verify device is present
if (auto result = pot.probe(); !result.has_value()) {
    // Handle error
    return -1;
}

// Set wiper 0 to midpoint (128 of 256)
pot.set_wiper(mcp44xx::Wiper::W0, 128);

// Save to non-volatile memory (persists after power cycle)
pot.set_nonvolatile_wiper(mcp44xx::Wiper::W0, 128);
```

## I2C Address Configuration

The MCP44XX uses a fixed part of the address (01011xxx). A1 and A0 pins select the lower 2 bits:

| A1 | A0 | 7-bit Address | Address Constant |
|----|----|---------------|------------------|
| 0  | 0  | 0x2C | `make_address(0, 0)` |
| 0  | 1  | 0x2D | `make_address(0, 1)` |
| 1  | 0  | 0x2E | `make_address(1, 0)` |
| 1  | 1  | 0x2F | `make_address(1, 1)` |

## Device Types

| Part | Wipers | Resolution | Max Code | Description |
|------|--------|------------|----------|-------------|
| MCP4441 | 1 | 7-bit | 0x80 (128) | Single pot, 7-bit |
| MCP4442 | 2 | 7-bit | 0x80 (128) | Dual pot, 7-bit |
| MCP4461 | 1 | 8-bit | 0x100 (256) | Single pot, 8-bit |
| MCP4462 | 2 | 8-bit | 0x100 (256) | Dual pot, 8-bit |

**Note**: All parts use the same driver. Select the appropriate `Resolution` based on your device.

## Register Map

| Address | Name | Description | Access |
|---------|------|-------------|--------|
| 0x00 | W0 | Wiper 0 volatile | R/W |
| 0x01 | W1 | Wiper 1 volatile | R/W |
| 0x02 | W0_NV | Wiper 0 non-volatile | R/W |
| 0x03 | W1_NV | Wiper 1 non-volatile | R/W |
| 0x04 | TCON0 | Tap control 0 | R/W |
| 0x05 | STATUS | Device status | R/O |
| 0x06 | W2 | Wiper 2 volatile | R/W |
| 0x07 | W3 | Wiper 3 volatile | R/W |
| 0x08 | W2_NV | Wiper 2 non-volatile | R/W |
| 0x09 | W3_NV | Wiper 3 non-volatile | R/W |
| 0x0A | TCON1 | Tap control 1 | R/W |
| 0x0B-0x0F | EE0-EE4 | EEPROM slots 0-4 | R/W |

**Note**: The MCP444x/446x uses non-sequential volatile wiper register addresses. Use the driver's API instead of direct register access.

## Wiper Register Mapping

| Wiper | Volatile Reg | Non-Volatile Reg |
|-------|--------------|------------------|
| W0 | 0x00 | 0x02 |
| W1 | 0x01 | 0x03 |
| W2 | 0x06 | 0x08 |
| W3 | 0x07 | 0x09 |

## API Reference

### Constructor & Device Info

| Method | Description |
|--------|-------------|
| `DigitalPotentiometer(hi2c, address7, resolution, timeout_ms)` | Create driver instance |
| `probe()` | Verify device presence on I2C bus |
| `get_address()` | Get 7-bit I2C address |
| `get_resolution()` | Get wiper resolution |
| `is_initialized()` | Check if driver is initialized |
| `max_wiper_code()` | Get maximum wiper code value |

### Volatile Wiper Operations

| Method | Description |
|--------|-------------|
| `set_wiper(Wiper, uint16_t)` | Set wiper value (SRAM) |
| `get_wiper(Wiper, uint16_t&)` | Get wiper value |
| `increment_wiper(Wiper)` | Increment wiper by 1 |
| `decrement_wiper(Wiper)` | Decrement wiper by 1 |

### Non-Volatile Wiper Operations

| Method | Description |
|--------|-------------|
| `set_nonvolatile_wiper(Wiper, uint16_t)` | Save wiper to EEPROM |
| `get_nonvolatile_wiper(Wiper, uint16_t&)` | Read wiper from EEPROM |
| `wait_eeprom_ready(timeout_ms)` | Wait for EEPROM write to complete |

### Status & Control

| Method | Description |
|--------|-------------|
| `read_status(Status&)` | Read status flags |
| `read_status_raw(uint16_t&)` | Read raw status register |
| `write_tcon0(uint16_t)` | Write TCON register 0 |
| `read_tcon0(uint16_t&)` | Read TCON register 0 |
| `write_tcon1(uint16_t)` | Write TCON register 1 |
| `read_tcon1(uint16_t&)` | Read TCON register 1 |

### EEPROM Operations

| Method | Description |
|--------|-------------|
| `write_eeprom(slot, value)` | Write to EEPROM slot (0-4) |
| `read_eeprom(slot, uint16_t&)` | Read from EEPROM slot |

### Status Flags

The `Status` struct contains:

| Field | Description |
|-------|-------------|
| `wl0-wl3` | Wiper write lock for each wiper |
| `eeprom_write_active` | EEPROM write cycle in progress |
| `write_protected` | Device write protection enabled |

## Complete Example

```cpp
#include "mcp44xx.hpp"
#include <iostream>

int main() {
    // Create I2C address for A1=1, A0=1 (0x2F)
    uint8_t address = mcp44xx::make_address(1, 1);
    
    // Initialize driver (8-bit resolution for MCP4461/4462)
    mcp44xx::DigitalPotentiometer pot(&hi2c1, address, mcp44xx::Resolution::Bits8);
    
    // Verify device is present
    if (auto result = pot.probe(); !result.has_value()) {
        std::cout << "Device not found!" << std::endl;
        return -1;
    }
    
    // Set all wipers to midpoint
    for (auto wiper : {mcp44xx::Wiper::W0, mcp44xx::Wiper::W1,
                       mcp44xx::Wiper::W2, mcp44xx::Wiper::W3}) {
        pot.set_wiper(wiper, 128);
        pot.set_nonvolatile_wiper(wiper, 128);
    }
    
    // Read back status
    mcp44xx::Status status;
    pot.read_status(status);
    
    std::cout << "Wiper 0: " << status.wl0 << std::endl;
    std::cout << "EEPROM busy: " << status.eeprom_write_active << std::endl;
    
    // Increment wiper 0
    pot.increment_wiper(mcp44xx::Wiper::W0);
    
    return 0;
}
```

## Advanced Examples

### Setting Potentiometer to 50% Resistance

```cpp
// For 8-bit (256 taps): 50% = 128
// For 7-bit (128 taps): 50% = 64
uint16_t midpoint = pot.max_wiper_code() / 2;
pot.set_wiper(mcp44xx::Wiper::W0, midpoint);
```

### Reading Wiper Value and Converting to Resistance

```cpp
uint16_t code;
pot.get_wiper(mcp44xx::Wiper::W0, code);

// Assume total resistance is 10kΩ
float total_resistance = 10000.0f;
float wiper_resistance = (static_cast<float>(code) / pot.max_wiper_code()) * total_resistance;

std::cout << "Wiper resistance: " << wiper_resistance << "Ω" << std::endl;
```

### Using EEPROM for Power-On Configuration

```cpp
// Save configuration to EEPROM
pot.set_nonvolatile_wiper(mcp44xx::Wiper::W0, 64);   // Power-on setting
pot.set_nonvolatile_wiper(mcp44xx::Wiper::W1, 128);

// On next power-up, copy from EEPROM to volatile
uint16_t code;
pot.get_nonvolatile_wiper(mcp44xx::Wiper::W0, code);
pot.set_wiper(mcp44xx::Wiper::W0, code);
```

### Error Handling

```cpp
auto result = pot.set_wiper(mcp44xx::Wiper::W0, 200);
if (!result.has_value()) {
    std::error_code ec = result.error();
    
    if (ec.category() == mcp44xx::get_error_category()) {
        switch (static_cast<mcp44xx::ErrorCode>(ec.value())) {
            case mcp44xx::ErrorCode::InvalidWiper:
                std::cout << "Invalid wiper number" << std::endl;
                break;
            case mcp44xx::ErrorCode::InvalidParameter:
                std::cout << "Invalid wiper code" << std::endl;
                break;
            case mcp44xx::ErrorCode::Timeout:
                std::cout << "I2C operation timed out" << std::endl;
                break;
            case mcp44xx::ErrorCode::HALError:
                std::cout << "HAL error" << std::endl;
                break;
        }
    }
}
```

## Troubleshooting

### Device Not Detected
- Verify I2C address matches A1/A0 pin configuration
- Check I2C pull-up resistors (typically 4.7kΩ)
- Verify I2C clock speed (100kHz or 400kHz)
- Check for bus contention with other devices

### Wiper Value Not Changing
- Verify wiper number is valid (W0-W3)
- Ensure code value is within range (0 to max_wiper_code())
- Check for write lock status (status.wlX flags)

### EEPROM Write Issues
- Wait for `wait_eeprom_ready()` before next operation
- EEPROM write takes ~5ms maximum
- Check `status.eeprom_write_active` flag

### Incorrect Register Access
- Use the driver's API instead of direct register access
- Volatile wiper registers are non-sequential (0x00, 0x01, 0x06, 0x07)

## License

MIT License - see LICENSE file for details
