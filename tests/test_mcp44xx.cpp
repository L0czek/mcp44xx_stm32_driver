/**
 * @file test_mcp44xx.cpp
 * @brief Unit tests for MCP44XX digital potentiometer driver
 */

#include "stub_hal.h"
#include "mcp44xx.hpp"
#include "mock_mcp44xx.hpp"
#include <cstdlib>
#include <iostream>
#include <string>

namespace {
int failures = 0;

void check(bool condition, const char* expr, const char* file, int line) {
    if (!condition) {
        ++failures;
        std::cerr << file << ':' << line << ": CHECK failed: " << expr << '\n';
    }
}

#define CHECK(x) check((x), #x, __FILE__, __LINE__)
#define CHECK_EQ(a, b) check(((a) == (b)), #a " == " #b, __FILE__, __LINE__)

struct Fixture {
    I2C_HandleTypeDef i2c{};
    MockMcp44xx dev;

    explicit Fixture(bool bits8 = true) : dev(0x2C, bits8) { HAL_ResetTick(); dev.attach(i2c); }
};

void test_address_and_probe() {
    CHECK_EQ(mcp44xx::make_address(0, 0), 0x2C);
    CHECK_EQ(mcp44xx::make_address(0, 1), 0x2D);
    CHECK_EQ(mcp44xx::make_address(1, 0), 0x2E);
    CHECK_EQ(mcp44xx::make_address(1, 1), 0x2F);

    Fixture f;
    mcp44xx::DigitalPotentiometer d(&f.i2c, 0x2C);
    CHECK(d.probe().has_value());
    f.dev.set_present(false);
    CHECK(!d.probe().has_value());
}

void test_8bit_volatile_wiper_protocol() {
    Fixture f(true);
    mcp44xx::DigitalPotentiometer d(&f.i2c, 0x2C, mcp44xx::Resolution::Bits8);
    CHECK_EQ(d.max_wiper_code(), 0x100);

    CHECK(d.set_wiper(mcp44xx::Wiper::W0, 0x0A5).has_value());
    CHECK_EQ(f.dev.volatile_wiper(0), 0x0A5);

    uint16_t value = 0;
    CHECK(d.get_wiper(mcp44xx::Wiper::W0, value).has_value());
    CHECK_EQ(value, 0x0A5);
}

void test_7bit_limits() {
    Fixture f(false);
    mcp44xx::DigitalPotentiometer d(&f.i2c, 0x2C, mcp44xx::Resolution::Bits7);
    CHECK_EQ(d.max_wiper_code(), 0x80);
    CHECK(d.set_wiper(mcp44xx::Wiper::W3, 0x80).has_value());
    CHECK(!d.set_wiper(mcp44xx::Wiper::W3, 0x81).has_value());
    CHECK_EQ(f.dev.volatile_wiper(3), 0x80);
}

void test_all_wipers_and_command_addresses() {
    Fixture f;
    mcp44xx::DigitalPotentiometer d(&f.i2c, 0x2C);
    const mcp44xx::Wiper wipers[] = {mcp44xx::Wiper::W0, mcp44xx::Wiper::W1,
                                     mcp44xx::Wiper::W2, mcp44xx::Wiper::W3};
    for (unsigned i = 0; i < 4; ++i) {
        f.dev.clear_transactions();
        CHECK(d.set_wiper(wipers[i], static_cast<uint16_t>(0x20 + i)).has_value());

        uint16_t value = 0;
        CHECK(d.get_wiper(wipers[i], value).has_value());
        CHECK_EQ(value, static_cast<uint16_t>(0x20 + i));
    }
}

void test_nonvolatile_wiper_addresses_and_values() {
    Fixture f;
    mcp44xx::DigitalPotentiometer d(&f.i2c, 0x2C);
    const mcp44xx::Wiper wipers[] = {mcp44xx::Wiper::W0, mcp44xx::Wiper::W1,
                                     mcp44xx::Wiper::W2, mcp44xx::Wiper::W3};

    for (unsigned i = 0; i < 4; ++i) {
        f.dev.clear_transactions();
        CHECK(d.set_nonvolatile_wiper(wipers[i], static_cast<uint16_t>(0x40 + i)).has_value());

        uint16_t value = 0;
        CHECK(d.get_nonvolatile_wiper(wipers[i], value).has_value());
        CHECK_EQ(value, static_cast<uint16_t>(0x40 + i));
    }
}

void test_increment_decrement_saturation() {
    Fixture f;
    mcp44xx::DigitalPotentiometer d(&f.i2c, 0x2C);
    CHECK(d.set_wiper(mcp44xx::Wiper::W1, 0x0).has_value());
    CHECK(d.decrement_wiper(mcp44xx::Wiper::W1).has_value());
    CHECK_EQ(f.dev.volatile_wiper(1), 0);

    CHECK(d.increment_wiper(mcp44xx::Wiper::W1).has_value());
    CHECK_EQ(f.dev.volatile_wiper(1), 1);

    CHECK(d.set_wiper(mcp44xx::Wiper::W1, 0x100).has_value());
    CHECK(d.increment_wiper(mcp44xx::Wiper::W1).has_value());
    CHECK_EQ(f.dev.volatile_wiper(1), 0x100);
}

void test_tcon() {
    Fixture f;
    mcp44xx::DigitalPotentiometer d(&f.i2c, 0x2C);
    CHECK(d.write_tcon0(0x0F3).has_value());
    CHECK_EQ(f.dev.tcon0(), 0x0F3);
    uint16_t value = 0;
    CHECK(d.read_tcon0(value).has_value());
    CHECK_EQ(value, 0x0F3);

    CHECK(d.write_tcon1(0x1FF).has_value());
    CHECK_EQ(f.dev.tcon1(), 0x1FF);
    CHECK(!d.write_tcon1(0x200).has_value());
}

void test_status_mapping() {
    Fixture f;
    mcp44xx::DigitalPotentiometer d(&f.i2c, 0x2C);
    f.dev.set_wiper_locked(0, true);
    f.dev.set_wiper_locked(2, true);
    f.dev.set_write_protected(true);

    mcp44xx::Status status{};
    CHECK(d.read_status(status).has_value());
    CHECK(status.wl0);
    CHECK(!status.wl1);
    CHECK(status.wl2);
    CHECK(!status.wl3);
    CHECK(!status.eeprom_write_active);
    CHECK(status.write_protected);
}

void test_eeprom_busy_polling_and_nv_wiper() {
    Fixture f;
    mcp44xx::DigitalPotentiometer d(&f.i2c, 0x2C);
    f.dev.set_eeprom_write_time(3);

    CHECK(d.set_nonvolatile_wiper(mcp44xx::Wiper::W2, 0x100).has_value());
    CHECK_EQ(f.dev.nonvolatile_wiper(2), 0x100);
    CHECK_EQ(f.dev.volatile_wiper(2), 0x80); // NV write does not directly alter volatile wiper.
    CHECK(HAL_GetTick() >= 3);

    uint16_t value = 0;
    CHECK(d.get_nonvolatile_wiper(mcp44xx::Wiper::W2, value).has_value());
    CHECK_EQ(value, 0x100);
}

void test_general_purpose_eeprom() {
    Fixture f;
    mcp44xx::DigitalPotentiometer d(&f.i2c, 0x2C);
    for (uint8_t slot = 0; slot < 5; ++slot) {
        CHECK(d.write_eeprom(slot, static_cast<uint16_t>(0x101 + slot)).has_value());
        uint16_t value = 0;
        CHECK(d.read_eeprom(slot, value).has_value());
        CHECK_EQ(value, static_cast<uint16_t>(0x101 + slot));
    }
    CHECK(!d.write_eeprom(5, 0).has_value());
    uint16_t invalid = 0;
    CHECK(!d.read_eeprom(5, invalid).has_value());
}

void test_eeprom_timeout() {
    Fixture f;
    mcp44xx::DigitalPotentiometer d(&f.i2c, 0x2C);
    f.dev.set_eeprom_write_time(100);
    CHECK(!d.write_eeprom(0, 0x123).has_value());
    CHECK(HAL_GetTick() >= 15);
}

void test_driver_input_validation_and_error_propagation() {
    Fixture f;
    mcp44xx::DigitalPotentiometer d(&f.i2c, 0x2C);
    CHECK(!d.set_wiper(mcp44xx::Wiper::W0, 0x101).has_value()); // 8-bit max is 0x100

    f.dev.set_next_error(HAL_TIMEOUT);
    uint16_t value = 0;
    CHECK(!d.get_wiper(mcp44xx::Wiper::W0, value).has_value());
}

void test_eeprom_write_protect() {
    Fixture f;
    mcp44xx::DigitalPotentiometer d(&f.i2c, 0x2C);
    f.dev.set_write_protected(true);
    CHECK(!d.write_eeprom(0, 0x123).has_value());
    CHECK_EQ(f.dev.eeprom(0), 0);
}

void test_read_raw_9bit_format() {
    Fixture f;
    mcp44xx::DigitalPotentiometer d(&f.i2c, 0x2C);
    CHECK(d.set_wiper(mcp44xx::Wiper::W0, 0x100).has_value());
    uint16_t value = 0;
    CHECK(d.get_wiper(mcp44xx::Wiper::W0, value).has_value());
    CHECK_EQ(value, 0x100);
}

} // namespace

int main() {
    test_address_and_probe();
    test_8bit_volatile_wiper_protocol();
    test_7bit_limits();
    test_all_wipers_and_command_addresses();
    test_nonvolatile_wiper_addresses_and_values();
    test_increment_decrement_saturation();
    test_tcon();
    test_status_mapping();
    test_eeprom_busy_polling_and_nv_wiper();
    test_eeprom_timeout();
    test_general_purpose_eeprom();
    test_driver_input_validation_and_error_propagation();
    test_eeprom_write_protect();
    test_read_raw_9bit_format();

    if (failures != 0) {
        std::cerr << failures << " test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "All MCP44XX tests passed\n";
    return EXIT_SUCCESS;
}
