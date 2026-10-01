#pragma once

#include "stub_hal.h"
#include <array>
#include <cstdint>
#include <vector>

class MockMcp44xx {
public:
    struct Transaction {
        enum class Type { WriteRegister, ReadRegister, CommandOnly };
        Type type;
        uint8_t command = 0;
        uint16_t value = 0;
        uint16_t dev_address = 0;
    };

    explicit MockMcp44xx(uint8_t address7 = 0x2C, bool bits8 = true);

    void attach(I2C_HandleTypeDef& handle);
    void reset();

    uint8_t address7() const { return address7_; }
    bool bits8() const { return bits8_; }
    uint16_t max_wiper_code() const { return bits8_ ? 0x100u : 0x80u; }

    uint16_t volatile_wiper(unsigned i) const;
    uint16_t nonvolatile_wiper(unsigned i) const;
    uint16_t tcon0() const;
    uint16_t tcon1() const;
    uint16_t eeprom(unsigned slot) const;

    void set_status_bits(uint16_t bits);
    void set_write_protected(bool enabled);
    void set_wiper_locked(unsigned i, bool enabled);

    void set_next_error(HAL_StatusTypeDef status);
    void set_present(bool present);

    // EEPROM busy simulation. The real device has a typical 3 ms and max 10 ms
    // write cycle; the mock uses the configured number of milliseconds.
    void set_eeprom_write_time(uint32_t ms) { eeprom_write_time_ms_ = ms; }
    bool eeprom_write_active() const;

    const std::vector<Transaction>& transactions() const { return transactions_; }
    void clear_transactions() { transactions_.clear(); }

private:
    friend HAL_StatusTypeDef HAL_I2C_IsDeviceReady(I2C_HandleTypeDef*, uint16_t, uint32_t, uint32_t);
    friend HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef*, uint16_t, uint16_t, uint16_t, uint8_t*, uint16_t, uint32_t);
    friend HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef*, uint16_t, uint16_t, uint16_t, uint8_t*, uint16_t, uint32_t);
    friend HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef*, uint16_t, uint8_t*, uint16_t, uint32_t);

    static constexpr uint8_t REG_W0_V = 0x00;
    static constexpr uint8_t REG_W1_V = 0x01;
    static constexpr uint8_t REG_TCON0 = 0x04;
    static constexpr uint8_t REG_STATUS = 0x05;
    static constexpr uint8_t REG_W2_V = 0x06;
    static constexpr uint8_t REG_W3_V = 0x07;
    static constexpr uint8_t REG_TCON1 = 0x0A;
    static constexpr uint8_t REG_W0_NV = 0x02;
    static constexpr uint8_t REG_W1_NV = 0x03;
    static constexpr uint8_t REG_W2_NV = 0x08;
    static constexpr uint8_t REG_W3_NV = 0x09;
    static constexpr uint8_t REG_EE0 = 0x0B;

    static constexpr uint8_t CMD_WRITE = 0x00;
    static constexpr uint8_t CMD_INC = 0x04;
    static constexpr uint8_t CMD_DEC = 0x08;
    static constexpr uint8_t CMD_READ = 0x0C;

    static constexpr uint16_t STATUS_EEWA = 1u << 4;
    static constexpr uint16_t STATUS_WP = 1u << 0;
    static constexpr uint16_t STATUS_WL0 = 1u << 2;
    static constexpr uint16_t STATUS_WL1 = 1u << 3;
    static constexpr uint16_t STATUS_WL2 = 1u << 5;
    static constexpr uint16_t STATUS_WL3 = 1u << 6;

    uint8_t address7_;
    bool bits8_;
    bool present_ = true;
    std::array<uint16_t, 4> vw_{};
    std::array<uint16_t, 4> nvw_{};
    std::array<uint16_t, 2> tcon_{};
    std::array<uint16_t, 5> eeprom_{};
    uint16_t status_feature_bits_ = 0;
    uint32_t eeprom_busy_until_ms_ = 0;
    uint32_t eeprom_write_time_ms_ = 3;
    HAL_StatusTypeDef next_error_ = HAL_OK;
    std::vector<Transaction> transactions_;

    uint16_t readRegister(uint8_t reg);
    HAL_StatusTypeDef writeCommand(uint8_t command, const uint8_t* data, uint16_t size);
    HAL_StatusTypeDef commandOnly(uint8_t command);

    uint8_t regFromCommand(uint8_t command) const { return static_cast<uint8_t>(command >> 4); }
    uint8_t opFromCommand(uint8_t command) const { return static_cast<uint8_t>(command & 0x0Cu); }
    uint16_t commandData(uint8_t command, const uint8_t* data, uint16_t size) const;
    bool addressMatches(uint16_t dev_address) const { return (dev_address >> 1) == address7_; }
    bool isVolatileReg(uint8_t reg) const;
    bool isNvReg(uint8_t reg) const;
    int wiperIndexFromVolatileReg(uint8_t reg) const;
    int wiperIndexFromNvReg(uint8_t reg) const;
    uint16_t& regRef(uint8_t reg);
    uint16_t statusValue() const;
    void startEepromWrite();
};
