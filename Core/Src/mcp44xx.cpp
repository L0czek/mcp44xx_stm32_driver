#include "mcp44xx.hpp"

namespace mcp44xx {

DigitalPotentiometer::DigitalPotentiometer(I2C_HandleTypeDef* hi2c,
                                          uint8_t address7,
                                          Resolution resolution,
                                          uint32_t timeout_ms)
    : hi2c_(hi2c),
      address7_(address7 & 0x7Fu),
      resolution_(resolution),
      timeout_ms_(timeout_ms),
      initialized_(true) {}

DigitalPotentiometer::~DigitalPotentiometer() = default;

std::error_code DigitalPotentiometer::convert_hal_status(HAL_StatusTypeDef status) {
    switch (status) {
        case HAL_OK:
            return make_error_code(ErrorCode::None);
        case HAL_TIMEOUT:
            return make_error_code(ErrorCode::Timeout);
        case HAL_ERROR:
        default:
            return make_error_code(ErrorCode::HALError);
    }
}

bool DigitalPotentiometer::valid_wiper(Wiper wiper) {
    return static_cast<uint8_t>(wiper) < 4u;
}

uint16_t DigitalPotentiometer::max_wiper_code() const {
    return resolution_ == Resolution::Bits7 ? 0x80u : 0x100u;
}

uint8_t DigitalPotentiometer::volatile_wiper_reg(Wiper wiper) const {
    static constexpr uint8_t map[] = {
        [static_cast<uint8_t>(Wiper::W0)] = REG_W0_V,
        [static_cast<uint8_t>(Wiper::W1)] = REG_W1_V,
        [static_cast<uint8_t>(Wiper::W2)] = REG_W2_V,
        [static_cast<uint8_t>(Wiper::W3)] = REG_W3_V
    };
    return map[static_cast<uint8_t>(wiper)];
}

uint8_t DigitalPotentiometer::nonvolatile_wiper_reg(Wiper wiper) const {
    switch (wiper) {
        case Wiper::W0: return REG_W0_NV;
        case Wiper::W1: return REG_W1_NV;
        case Wiper::W2: return REG_W2_NV;
        case Wiper::W3: return REG_W3_NV;
        default: return 0xFF;
    }
}

mcp44xx::expected<void> DigitalPotentiometer::probe() {
    const HAL_StatusTypeDef status = HAL_I2C_IsDeviceReady(
        hi2c_,
        static_cast<uint16_t>(address7_ << 1),
        2,
        timeout_ms_
    );
    if (status != HAL_OK) {
        return convert_hal_status(status);
    }
    return {};
}

mcp44xx::expected<void> DigitalPotentiometer::write_register(uint8_t reg, uint16_t value) {
    // The command byte contains D8 in bit 0; D9 is currently unused.
    const uint8_t command = static_cast<uint8_t>((reg << 4) | CMD_WRITE |
                                                   ((value >> 8) & 0x01u));
    uint8_t data = static_cast<uint8_t>(value & 0xFFu);

    const HAL_StatusTypeDef status = HAL_I2C_Mem_Write(
        hi2c_,
        static_cast<uint16_t>(address7_ << 1),
        command,
        I2C_MEMADD_SIZE_8BIT,
        &data,
        1,
        timeout_ms_
    );

    if (status != HAL_OK) {
        return convert_hal_status(status);
    }
    return {};
}

mcp44xx::expected<void> DigitalPotentiometer::read_register(uint8_t reg, uint16_t& value) {
    const uint8_t command = static_cast<uint8_t>((reg << 4) | CMD_READ);
    uint8_t data[2] = {0, 0};

    const HAL_StatusTypeDef status = HAL_I2C_Mem_Read(
        hi2c_,
        static_cast<uint16_t>(address7_ << 1),
        command,
        I2C_MEMADD_SIZE_8BIT,
        data,
        2,
        timeout_ms_
    );

    if (status != HAL_OK) {
        return convert_hal_status(status);
    }

    // Read format is: [0..0 D8] [D7..D0].
    value = static_cast<uint16_t>(((data[0] & 0x01u) << 8) | data[1]);
    return {};
}

mcp44xx::expected<void> DigitalPotentiometer::command_only(uint8_t reg, uint8_t command) {
    uint8_t command_byte = static_cast<uint8_t>((reg << 4) | command);

    const HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(
        hi2c_,
        static_cast<uint16_t>(address7_ << 1),
        &command_byte,
        1,
        timeout_ms_
    );

    if (status != HAL_OK) {
        return convert_hal_status(status);
    }
    return {};
}

mcp44xx::expected<void> DigitalPotentiometer::set_wiper(Wiper wiper, uint16_t code) {
    if (!valid_wiper(wiper)) {
        return make_error_code(ErrorCode::InvalidWiper);
    }
    if (code > max_wiper_code()) {
        return make_error_code(ErrorCode::InvalidParameter);
    }

    return write_register(volatile_wiper_reg(wiper), code);
}

mcp44xx::expected<void> DigitalPotentiometer::get_wiper(Wiper wiper, uint16_t& code) {
    if (!valid_wiper(wiper)) {
        return make_error_code(ErrorCode::InvalidWiper);
    }

    return read_register(volatile_wiper_reg(wiper), code);
}

mcp44xx::expected<void> DigitalPotentiometer::set_nonvolatile_wiper(Wiper wiper, uint16_t code) {
    if (!valid_wiper(wiper)) {
        return make_error_code(ErrorCode::InvalidWiper);
    }
    if (code > max_wiper_code()) {
        return make_error_code(ErrorCode::InvalidParameter);
    }

    const auto ready = wait_eeprom_ready();
    if (!ready.has_value()) {
        return ready.error();
    }

    const auto status = write_register(nonvolatile_wiper_reg(wiper), code);
    if (!status.has_value()) {
        return status.error();
    }

    return wait_eeprom_ready();
}

mcp44xx::expected<void> DigitalPotentiometer::get_nonvolatile_wiper(Wiper wiper, uint16_t& code) {
    if (!valid_wiper(wiper)) {
        return make_error_code(ErrorCode::InvalidWiper);
    }

    return read_register(nonvolatile_wiper_reg(wiper), code);
}

mcp44xx::expected<void> DigitalPotentiometer::increment_wiper(Wiper wiper) {
    if (!valid_wiper(wiper)) {
        return make_error_code(ErrorCode::InvalidWiper);
    }

    return command_only(volatile_wiper_reg(wiper), CMD_INC);
}

mcp44xx::expected<void> DigitalPotentiometer::decrement_wiper(Wiper wiper) {
    if (!valid_wiper(wiper)) {
        return make_error_code(ErrorCode::InvalidWiper);
    }

    return command_only(volatile_wiper_reg(wiper), CMD_DEC);
}

mcp44xx::expected<void> DigitalPotentiometer::read_status_raw(uint16_t& value) {
    return read_register(REG_STATUS, value);
}

mcp44xx::expected<void> DigitalPotentiometer::read_status(Status& status) {
    uint16_t value = 0;
    const auto result = read_status_raw(value);
    if (!result.has_value()) {
        return result.error();
    }

    status.wl0 = (value & STATUS_WL0) != 0;
    status.wl1 = (value & STATUS_WL1) != 0;
    status.wl2 = (value & STATUS_WL2) != 0;
    status.wl3 = (value & STATUS_WL3) != 0;
    status.eeprom_write_active = (value & STATUS_EEWA) != 0;
    status.write_protected = (value & STATUS_WP) != 0;
    return {};
}

mcp44xx::expected<void> DigitalPotentiometer::wait_eeprom_ready(uint32_t timeout_ms) {
    const uint32_t start = HAL_GetTick();

    for (;;) {
        uint16_t status = 0;
        const auto result = read_status_raw(status);
        if (!result.has_value()) {
            return result.error();
        }

        if ((status & STATUS_EEWA) == 0) {
            return {};
        }

        if ((HAL_GetTick() - start) >= timeout_ms) {
            return make_error_code(ErrorCode::Timeout);
        }

        HAL_Delay(1);
    }
}

mcp44xx::expected<void> DigitalPotentiometer::write_tcon0(uint16_t value) {
    if (value > 0x1FFu) {
        return make_error_code(ErrorCode::InvalidParameter);
    }
    return write_register(REG_TCON0, value);
}

mcp44xx::expected<void> DigitalPotentiometer::read_tcon0(uint16_t& value) {
    return read_register(REG_TCON0, value);
}

mcp44xx::expected<void> DigitalPotentiometer::write_tcon1(uint16_t value) {
    if (value > 0x1FFu) {
        return make_error_code(ErrorCode::InvalidParameter);
    }
    return write_register(REG_TCON1, value);
}

mcp44xx::expected<void> DigitalPotentiometer::read_tcon1(uint16_t& value) {
    return read_register(REG_TCON1, value);
}

mcp44xx::expected<void> DigitalPotentiometer::write_eeprom(uint8_t slot, uint16_t value) {
    if (slot >= 5) {
        return make_error_code(ErrorCode::InvalidSlot);
    }
    if (value > 0x1FFu) {
        return make_error_code(ErrorCode::InvalidParameter);
    }

    const auto ready = wait_eeprom_ready();
    if (!ready.has_value()) {
        return ready.error();
    }

    const auto status = write_register(static_cast<uint8_t>(REG_EE0 + slot), value);
    if (!status.has_value()) {
        return status.error();
    }

    return wait_eeprom_ready();
}

mcp44xx::expected<void> DigitalPotentiometer::read_eeprom(uint8_t slot, uint16_t& value) {
    if (slot >= 5) {
        return make_error_code(ErrorCode::InvalidSlot);
    }
    return read_register(static_cast<uint8_t>(REG_EE0 + slot), value);
}

} // namespace mcp44xx
