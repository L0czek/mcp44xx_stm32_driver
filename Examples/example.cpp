#include "mcp44xx.hpp"

extern I2C_HandleTypeDef hi2c1;

void digipot_init_and_test()
{
    // A1=0, A0=0 -> 7-bit I2C address 0x2C.
    mcp44xx::DigitalPotentiometer pot(&hi2c1, mcp44xx::make_address(0, 0), mcp44xx::Resolution::Bits8);

    if (auto err = pot.probe(); !err.has_value()) {
        return;
    }

    // 8-bit MCP446x: 0x100 is full-scale, 0x080 is mid-scale.
    (void)pot.set_wiper(mcp44xx::Wiper::W0, 0x080);

    uint16_t code = 0;
    (void)pot.get_wiper(mcp44xx::Wiper::W0, code);

    // Persist W0 in EEPROM. This starts an EEPROM write cycle.
    (void)pot.set_nonvolatile_wiper(mcp44xx::Wiper::W0, 0x080);
}
