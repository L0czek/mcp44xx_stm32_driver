# MCP44XX Digital Potentiometer Driver

I2C driver for the Microchip MCP444x/446x series of digital potentiometers for STM32 microcontrollers.

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

## API

See `../Core/Inc/mcp44xx.hpp` for the complete API reference.

## Example Usage

```cpp
#include "mcp44xx.hpp"

// Initialize driver with I2C handle and address
DigitalPotentiometer pot(&hi2c, 0x2F);  // I2C address 0x2F

// Check if device is present
if (auto result = pot.probe(); result.has_value()) {
    // Device is ready
}

// Set wiper value (0-255 for 8-bit resolution)
pot.set_wiper(DigitalPotentiometer::Wiper::W0, 128);

// Read wiper value
if (auto value = pot.get_wiper(DigitalPotentiometer::Wiper::W0); value.has_value()) {
    // value.value() returns wiper value (0-255)
}
```

## I2C Address Configuration

The MCP44XX supports multiple I2C addresses based on A0 and A1 pin strapping:
- `0x2F` (default)
- Other addresses available via pin configuration

## Device Types

- **MCP4441**: 1 pot, 257 wiper positions (8-bit)
- **MCP4442**: 2 pots, 257 wiper positions each (8-bit)
- **MCP4461**: 1 pot, 257 wiper positions (8-bit)
- **MCP4462**: 2 pots, 257 wiper positions each (8-bit)

## Features

- Individual wiper control for each potentiometer
- Wiper value read/write (0-255)
- Device probe and identity verification
- Non-volatile wiper storage support

## Register Map

| Register | Name | Description |
|----------|------|-------------|
| 0x00 | W0 | Wiper 0 volatile register |
| 0x01 | W1 | Wiper 1 volatile register |
| 0x06 | W2 | Wiper 2 volatile register |
| 0x07 | W3 | Wiper 3 volatile register |
| 0x02 | W0_NV | Wiper 0 non-volatile register |
| 0x03 | W1_NV | Wiper 1 non-volatile register |
| 0x04 | TAP0 | Tap 0 register |
| 0x05 | TAP1 | Tap 1 register |
| 0x08 | W2_NV | Wiper 2 non-volatile register |
| 0x09 | W3_NV | Wiper 3 non-volatile register |

**Note**: The MCP444x/446x uses non-sequential register addresses. Use the driver's API instead of direct register access.

## Wiper Mapping

| Wiper Enum | Volatile Register | Non-Volatile Register |
|------------|-------------------|----------------------|
| W0 | 0x00 | 0x02 |
| W1 | 0x01 | 0x03 |
| W2 | 0x06 | 0x08 |
| W3 | 0x07 | 0x09 |
