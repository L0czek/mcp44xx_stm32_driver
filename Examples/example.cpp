#include "mcp44xx.hpp"

extern I2C_HandleTypeDef hi2c1;

void digipot_init_and_test()
{
    // A1=0, A0=0 -> 7-bit I2C address 0x2C.
    // MCP4441 (board part) is a 7-bit device: 129 steps, full scale 0x80, mid-scale 0x40.
    mcp44xx::DigitalPotentiometer pot(&hi2c1, mcp44xx::make_address(0, 0), mcp44xx::Resolution::Bits7);

    if (auto err = pot.probe(); !err.has_value()) {
        return;
    }

    (void)pot.set_wiper(mcp44xx::Wiper::W0, pot.mid_scale_code());

    uint16_t code = 0;
    (void)pot.get_wiper(mcp44xx::Wiper::W0, code);

    // Persist W0 in EEPROM. This starts an EEPROM write cycle (tWC up to 10 ms).
    (void)pot.set_nonvolatile_wiper(mcp44xx::Wiper::W0, pot.mid_scale_code());
}
