#include "stub_hal.h"
#include "mock_mcp44xx.hpp"
#include <algorithm>

namespace {
MockMcp44xx* mockFor(I2C_HandleTypeDef* h) {
    return h ? static_cast<MockMcp44xx*>(h->UserData) : nullptr;
}
}

static uint32_t g_tick = 0;

MockMcp44xx::MockMcp44xx(uint8_t address7, bool bits8)
    : address7_(static_cast<uint8_t>(address7 & 0x7Fu)), bits8_(bits8) {
    reset();
}

void MockMcp44xx::attach(I2C_HandleTypeDef& handle) {
    handle.UserData = this;
}

void MockMcp44xx::reset() {
    vw_.fill(bits8_ ? 0x80u : 0x40u);
    nvw_.fill(bits8_ ? 0x80u : 0x40u);
    tcon_.fill(0x1FFu);
    eeprom_.fill(0);
    status_feature_bits_ = 0;
    eeprom_busy_until_ms_ = 0;
    next_error_ = HAL_OK;
    transactions_.clear();
}

uint16_t MockMcp44xx::volatile_wiper(unsigned i) const { return vw_.at(i); }
uint16_t MockMcp44xx::nonvolatile_wiper(unsigned i) const { return nvw_.at(i); }
uint16_t MockMcp44xx::tcon0() const { return tcon_[0]; }
uint16_t MockMcp44xx::tcon1() const { return tcon_[1]; }
uint16_t MockMcp44xx::eeprom(unsigned slot) const { return eeprom_.at(slot); }

void MockMcp44xx::set_status_bits(uint16_t bits) { status_feature_bits_ = static_cast<uint16_t>(bits & 0x007Du); }
void MockMcp44xx::set_write_protected(bool enabled) {
    if (enabled) status_feature_bits_ |= STATUS_WP;
    else status_feature_bits_ &= ~STATUS_WP;
}
void MockMcp44xx::set_wiper_locked(unsigned i, bool enabled) {
    const uint16_t bit = static_cast<uint16_t>(1u << (i == 0 ? 2 : i == 1 ? 3 : i == 2 ? 5 : 6));
    if (enabled) status_feature_bits_ |= bit;
    else status_feature_bits_ &= static_cast<uint16_t>(~bit);
}
bool MockMcp44xx::eeprom_write_active() const { return g_tick < eeprom_busy_until_ms_; }

void MockMcp44xx::set_next_error(HAL_StatusTypeDef status) { next_error_ = status; }
void MockMcp44xx::set_present(bool present) { present_ = present; }

bool MockMcp44xx::isVolatileReg(uint8_t reg) const {
    return reg == REG_W0_V || reg == REG_W1_V || reg == REG_TCON0 ||
           reg == REG_STATUS || reg == REG_W2_V || reg == REG_W3_V || reg == REG_TCON1;
}

bool MockMcp44xx::isNvReg(uint8_t reg) const {
    return (reg >= REG_W0_NV && reg <= REG_W1_NV) ||
           reg == REG_W2_NV || reg == REG_W3_NV ||
           (reg >= REG_EE0 && reg <= 0x0Fu);
}

int MockMcp44xx::wiperIndexFromVolatileReg(uint8_t reg) const {
    switch (reg) {
    case REG_W0_V: return 0;
    case REG_W1_V: return 1;
    case REG_W2_V: return 2;
    case REG_W3_V: return 3;
    default: return -1;
    }
}

int MockMcp44xx::wiperIndexFromNvReg(uint8_t reg) const {
    switch (reg) {
    case REG_W0_NV: return 0;
    case REG_W1_NV: return 1;
    case REG_W2_NV: return 2;
    case REG_W3_NV: return 3;
    default: return -1;
    }
}

uint16_t& MockMcp44xx::regRef(uint8_t reg) {
    if (const int i = wiperIndexFromVolatileReg(reg); i >= 0) return vw_[i];
    if (const int i = wiperIndexFromNvReg(reg); i >= 0) return nvw_[i];
    switch (reg) {
    case REG_TCON0: return tcon_[0];
    case REG_TCON1: return tcon_[1];
    default: break;
    }
    if (reg >= REG_EE0 && reg <= 0x0F) return eeprom_[reg - REG_EE0];
    return vw_[0]; // Never used for invalid registers.
}

uint16_t MockMcp44xx::statusValue() const {
    uint16_t value = static_cast<uint16_t>(status_feature_bits_ | (1u << 1) | (1u << 7) | (1u << 8));
    if (g_tick < eeprom_busy_until_ms_) value |= STATUS_EEWA;
    return value;
}

uint16_t MockMcp44xx::readRegister(uint8_t reg) {
    if (reg == REG_STATUS) return statusValue();
    if (wiperIndexFromVolatileReg(reg) >= 0 || wiperIndexFromNvReg(reg) >= 0 ||
        reg == REG_TCON0 || reg == REG_TCON1 || (reg >= REG_EE0 && reg <= 0x0F)) {
        return static_cast<uint16_t>(regRef(reg) & 0x01FFu);
    }
    return 0;
}

uint16_t MockMcp44xx::commandData(uint8_t command, const uint8_t* data, uint16_t size) const {
    if (size != 1 || data == nullptr) return 0;
    return static_cast<uint16_t>(((command & 0x01u) << 8) | data[0]);
}

void MockMcp44xx::startEepromWrite() {
    eeprom_busy_until_ms_ = g_tick + eeprom_write_time_ms_;
}

HAL_StatusTypeDef MockMcp44xx::writeCommand(uint8_t command, const uint8_t* data, uint16_t size) {
    const uint8_t reg = regFromCommand(command);
    const uint8_t op = opFromCommand(command);

    if (op != CMD_WRITE || !data || size != 1) return HAL_ERROR;
    const uint16_t value = commandData(command, data, size);
    if ((wiperIndexFromVolatileReg(reg) >= 0 && value > max_wiper_code()) ||
        value > 0x1FFu) return HAL_ERROR;

    if (reg == REG_STATUS || reg == 0x0B || reg == 0x0C || reg == 0x0D || reg == 0x0E || reg == 0x0F ||
        wiperIndexFromNvReg(reg) >= 0) {
        if (g_tick < eeprom_busy_until_ms_) return HAL_BUSY;
        if ((statusValue() & STATUS_WP) != 0) return HAL_ERROR;
    }

    if (const int i = wiperIndexFromVolatileReg(reg); i >= 0) {
        if ((status_feature_bits_ >> (i == 0 ? 2 : i == 1 ? 3 : i == 2 ? 5 : 6)) & 1u) return HAL_ERROR;
        vw_[i] = static_cast<uint16_t>(value & 0x01FFu);
        return HAL_OK;
    }
    if (const int i = wiperIndexFromNvReg(reg); i >= 0) {
        if ((status_feature_bits_ >> (i == 0 ? 2 : i == 1 ? 3 : i == 2 ? 5 : 6)) & 1u) return HAL_ERROR;
        nvw_[i] = static_cast<uint16_t>(value & 0x01FFu);
        startEepromWrite();
        return HAL_OK;
    }
    if (reg == REG_TCON0 || reg == REG_TCON1) {
        const int i = reg == REG_TCON0 ? 0 : 1;
        tcon_[i] = static_cast<uint16_t>(value & 0x01FFu);
        return HAL_OK;
    }
    if (reg >= REG_EE0 && reg <= 0x0F) {
        eeprom_[reg - REG_EE0] = static_cast<uint16_t>(value & 0x01FFu);
        startEepromWrite();
        return HAL_OK;
    }
    return HAL_ERROR;
}

HAL_StatusTypeDef MockMcp44xx::commandOnly(uint8_t command) {
    const uint8_t reg = regFromCommand(command);
    const uint8_t op = opFromCommand(command);
    if (op != CMD_INC && op != CMD_DEC) return HAL_ERROR;
    const int i = wiperIndexFromVolatileReg(reg);
    if (i < 0) return HAL_ERROR;
    if ((statusValue() & static_cast<uint16_t>(1u << (i == 0 ? 2 : i == 1 ? 3 : i == 2 ? 5 : 6))) != 0) return HAL_ERROR;

    if (op == CMD_INC) {
        if (vw_[i] < max_wiper_code()) ++vw_[i];
    } else {
        if (vw_[i] > 0) --vw_[i];
    }
    return HAL_OK;
}

HAL_StatusTypeDef HAL_I2C_IsDeviceReady(I2C_HandleTypeDef* hi2c, uint16_t DevAddress,
                                         uint32_t, uint32_t) {
    auto* m = mockFor(hi2c);
    if (!m || !m->present_ || !m->addressMatches(DevAddress)) return HAL_ERROR;
    if (m->next_error_ != HAL_OK) { const auto e = m->next_error_; m->next_error_ = HAL_OK; return e; }
    return HAL_OK;
}

HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef* hi2c, uint16_t DevAddress,
                                     uint16_t MemAddress, uint16_t MemAddSize,
                                     uint8_t* pData, uint16_t Size, uint32_t) {
    auto* m = mockFor(hi2c);
    if (!m || !m->present_ || !m->addressMatches(DevAddress) || MemAddSize != I2C_MEMADD_SIZE_8BIT) return HAL_ERROR;
    if (m->next_error_ != HAL_OK) { const auto e = m->next_error_; m->next_error_ = HAL_OK; return e; }
    if (Size != 1 || pData == nullptr) return HAL_ERROR;

    m->transactions_.push_back({MockMcp44xx::Transaction::Type::WriteRegister,
                                 static_cast<uint8_t>(MemAddress),
                                 static_cast<uint16_t>(((MemAddress & 1u) << 8) | pData[0]), DevAddress});
    return m->writeCommand(static_cast<uint8_t>(MemAddress), pData, Size);
}

HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef* hi2c, uint16_t DevAddress,
                                    uint16_t MemAddress, uint16_t MemAddSize,
                                    uint8_t* pData, uint16_t Size, uint32_t) {
    auto* m = mockFor(hi2c);
    if (!m || !m->present_ || !m->addressMatches(DevAddress) || MemAddSize != I2C_MEMADD_SIZE_8BIT) return HAL_ERROR;
    if (m->next_error_ != HAL_OK) { const auto e = m->next_error_; m->next_error_ = HAL_OK; return e; }
    if (Size != 2 || !pData) return HAL_ERROR;

    const uint8_t command = static_cast<uint8_t>(MemAddress);
    if ((command & 0x0Cu) != MockMcp44xx::CMD_READ) return HAL_ERROR;
    const uint8_t reg = static_cast<uint8_t>(command >> 4);
    const uint16_t value = m->readRegister(reg);
    m->transactions_.push_back({MockMcp44xx::Transaction::Type::ReadRegister, command, value, DevAddress});
    pData[0] = static_cast<uint8_t>((value >> 8) & 0x01u);
    pData[1] = static_cast<uint8_t>(value & 0xFFu);
    return HAL_OK;
}

HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef* hi2c, uint16_t DevAddress,
                                          uint8_t* pData, uint16_t Size, uint32_t) {
    auto* m = mockFor(hi2c);
    if (!m || !m->present_ || !m->addressMatches(DevAddress)) return HAL_ERROR;
    if (m->next_error_ != HAL_OK) { const auto e = m->next_error_; m->next_error_ = HAL_OK; return e; }
    if (Size != 1 || !pData) return HAL_ERROR;
    m->transactions_.push_back({MockMcp44xx::Transaction::Type::CommandOnly, pData[0], 0, DevAddress});
    return m->commandOnly(pData[0]);
}

uint32_t HAL_GetTick(void) { return g_tick; }
void HAL_Delay(uint32_t Delay) { g_tick += Delay; }
void HAL_ResetTick(void) { g_tick = 0; }
