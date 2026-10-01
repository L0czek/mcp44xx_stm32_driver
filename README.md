# MCP44XX Digital Potentiometer Driver

I2C driver for the Microchip MCP444x/446x series of digital potentiometers for STM32 microcontrollers.

[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![C++ Standard](https://img.shields.io/badge/C++17-blue.svg)](https://en.cppreference.com/w/cpp/17)

## Features

- 4 wipers (W0-W3) with independent control
- 7-bit or 8-bit resolution (selectable)
- Volatile and non-volatile wiper storage
- Increment/decrement operations
- EEPROM management with automatic busy-wait
- Status monitoring (write lock, EEPROM busy)
- TCON register access
- 5 programmable EEPROM slots

## Documentation

- [Library Documentation](library/README.md) - Integration and API reference
- [Header File](Core/Inc/mcp44xx.hpp) - Complete API reference

## Directory Structure

```
mcp44xx_stm32_driver/
├── library/              # CMake submodule integration
│   ├── CMakeLists.txt
│   └── README.md
├── Core/
│   ├── Inc/             # Public headers
│   │   └── mcp44xx.hpp
│   └── Src/             # Implementation
│       ├── mcp44xx.cpp
│       └── mcp44xx_expected.hpp
├── examples/            # Example code
├── tests/               # Unit tests
└── README.md
```

## Quick Start

```cpp
#include "mcp44xx.hpp"

uint8_t address = mcp44xx::make_address(0, 0);  // 0x2C
mcp44xx::DigitalPotentiometer pot(&hi2c, address, mcp44xx::Resolution::Bits8);
pot.probe();
pot.set_wiper(mcp44xx::Wiper::W0, 128);
pot.set_nonvolatile_wiper(mcp44xx::Wiper::W0, 128);
```

## Building

```bash
mkdir build && cd build
cmake ..
make
make test
```

## Unit Tests

Run tests from build directory:
```bash
./mcp44xx_tests
```

All 14 tests pass.

## License

MIT License
