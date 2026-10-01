/**
 * @file mcp44xx.hpp
 * @brief MCP4441/4442/4461/4462 I2C Digital Potentiometer Driver for STM32
 *
 * This driver provides a modern C++ interface for the Microchip MCP444x/446x
 * series of digital potentiometers with I2C interface.
 */

#ifndef __MCP44XX_HPP
#define __MCP44XX_HPP

#include <cstdint>
#include <cstddef>
#include <array>
#include <string>
#include <cstring>
#include <stdexcept>
#include <system_error>

// Include custom expected for C++17 compatibility
// This is namespace-specific to avoid duplicate definitions
#include "mcp44xx_expected.hpp"

// STM32 HAL includes - use stub for testing
#if defined(TESTING)
    // Testing mode: include stub HAL
    #include "stub_hal.h"
#else
    // Production: include real STM32 HAL
    #include "stm32g4xx_hal.h"
    #include "stm32g4xx_hal_i2c.h"
#endif

namespace mcp44xx {

/**
 * @brief MCP44XX address configuration
 * A6:A2 are fixed to 01011b, so valid 7-bit addresses are 0x2C..0x2F
 */
static constexpr uint8_t MCP44XX_BASE_ADDRESS = 0x2Cu;

/**
 * @brief Create a 7-bit I2C address from A1 and A0 pins
 * @param a1 A1 pin state (0 or 1)
 * @param a0 A0 pin state (0 or 1)
 * @return 7-bit I2C address
 */
static constexpr uint8_t make_address(uint8_t a1, uint8_t a0) {
    return static_cast<uint8_t>(MCP44XX_BASE_ADDRESS | ((a1 & 1u) << 1) | (a0 & 1u));
}

/**
 * @brief Wiper resolution configuration
 */
enum class Resolution : uint8_t {
    Bits7 = 0,  //!< MCP444x parts (7-bit wiper, max code 0x80)
    Bits8 = 1,  //!< MCP446x parts (8-bit wiper, max code 0x100)
};

/**
 * @brief Error codes for MCP44XX operations
 */
enum class ErrorCode : uint8_t {
    None = 0,
    HALError,
    Timeout,
    I2CError,
    InvalidAddress,
    InvalidParameter,
    InvalidWiper,
    InvalidSlot,
    EEPROMBusy
};

/**
 * @brief Custom error_category implementation for std::expected
 */
class MCP44XXErrorCategory : public std::error_category {
public:
    const char* name() const noexcept override {
        return "mcp44xx";
    }

    std::string message(int ev) const override {
        switch (static_cast<ErrorCode>(ev)) {
            case ErrorCode::None: return "No error";
            case ErrorCode::HALError: return "HAL error";
            case ErrorCode::Timeout: return "Operation timeout";
            case ErrorCode::I2CError: return "I2C communication error";
            case ErrorCode::InvalidAddress: return "Invalid I2C address";
            case ErrorCode::InvalidParameter: return "Invalid parameter";
            case ErrorCode::InvalidWiper: return "Invalid wiper number";
            case ErrorCode::InvalidSlot: return "Invalid EEPROM slot";
            case ErrorCode::EEPROMBusy: return "EEPROM write in progress";
            default: return "Unknown error";
        }
    }
};

/**
 * @brief Get the MCP44XX error category instance
 */
inline const MCP44XXErrorCategory& get_error_category() {
    static MCP44XXErrorCategory instance;
    return instance;
}

/**
 * @brief Create an error code from ErrorCode enum
 */
inline std::error_code make_error_code(ErrorCode ec) {
    return std::error_code(static_cast<int>(ec), get_error_category());
}

/**
 * @brief Wiper selection
 */
enum class Wiper : uint8_t {
    W0 = 0,
    W1 = 1,
    W2 = 2,
    W3 = 3,
};

/**
 * @brief Status flags from the device
 */
struct Status {
    bool wl0;                    //!< Wiper 0 write lock
    bool wl1;                    //!< Wiper 1 write lock
    bool wl2;                    //!< Wiper 2 write lock
    bool wl3;                    //!< Wiper 3 write lock
    bool eeprom_write_active;    //!< EEPROM write cycle in progress
    bool write_protected;        //!< Device write protected
};

/**
 * @brief MCP44XX Digital Potentiometer Driver Class
 *
 * This class provides a modern C++ interface for the Microchip MCP444x/446x
 * series of digital potentiometers. It supports 7-bit (MCP444x) and 8-bit
 * (MCP446x) resolution wipers with I2C interface.
 */
class DigitalPotentiometer {
public:
    /**
     * @brief Construct MCP44XX driver
     * @param hi2c Pointer to I2C handle
     * @param address7 7-bit I2C address (use mcp44xx_make_address() to create)
     * @param resolution Wiper resolution (Bits7 for MCP444x, Bits8 for MCP446x)
     * @param timeout_ms Command timeout in milliseconds
     */
    DigitalPotentiometer(I2C_HandleTypeDef* hi2c,
                        uint8_t address7,
                        Resolution resolution = Resolution::Bits8,
                        uint32_t timeout_ms = 100);

    /**
     * @brief Destroy MCP44XX driver
     */
    ~DigitalPotentiometer();

    /**
     * @brief Probe device presence on I2C bus
     * @return mcp44xx::expected<void> - error on failure
     */
    mcp44xx::expected<void> probe();

    /**
     * @brief Get maximum wiper code value
     * @return Maximum wiper code (0x80 for 7-bit, 0x100 for 8-bit)
     */
    uint16_t max_wiper_code() const;

    /**
     * @brief Set wiper value (volatile)
     * @param wiper Wiper to set
     * @param code Wiper code value
     * @return mcp44xx::expected<void> - error on failure
     */
    mcp44xx::expected<void> set_wiper(Wiper wiper, uint16_t code);

    /**
     * @brief Get wiper value (volatile)
     * @param wiper Wiper to read
     * @param code Pointer to store wiper code
     * @return mcp44xx::expected<void> - error on failure
     */
    mcp44xx::expected<void> get_wiper(Wiper wiper, uint16_t& code);

    /**
     * @brief Set wiper value in non-volatile memory
     * @param wiper Wiper to set
     * @param code Wiper code value
     * @return mcp44xx::expected<void> - error on failure
     */
    mcp44xx::expected<void> set_nonvolatile_wiper(Wiper wiper, uint16_t code);

    /**
     * @brief Get wiper value from non-volatile memory
     * @param wiper Wiper to read
     * @param code Pointer to store wiper code
     * @return mcp44xx::expected<void> - error on failure
     */
    mcp44xx::expected<void> get_nonvolatile_wiper(Wiper wiper, uint16_t& code);

    /**
     * @brief Increment wiper value by 1
     * @param wiper Wiper to increment
     * @return mcp44xx::expected<void> - error on failure
     */
    mcp44xx::expected<void> increment_wiper(Wiper wiper);

    /**
     * @brief Decrement wiper value by 1
     * @param wiper Wiper to decrement
     * @return mcp44xx::expected<void> - error on failure
     */
    mcp44xx::expected<void> decrement_wiper(Wiper wiper);

    /**
     * @brief Read device status
     * @param status Reference to Status struct to fill
     * @return mcp44xx::expected<void> - error on failure
     */
    mcp44xx::expected<void> read_status(Status& status);

    /**
     * @brief Read raw status register value
     * @param value Pointer to store raw status value
     * @return mcp44xx::expected<void> - error on failure
     */
    mcp44xx::expected<void> read_status_raw(uint16_t& value);

    /**
     * @brief Write TCON register 0
     * @param value TCON value (9-bit, 0x000-0x1FF)
     * @return mcp44xx::expected<void> - error on failure
     */
    mcp44xx::expected<void> write_tcon0(uint16_t value);

    /**
     * @brief Read TCON register 0
     * @param value Pointer to store TCON value
     * @return mcp44xx::expected<void> - error on failure
     */
    mcp44xx::expected<void> read_tcon0(uint16_t& value);

    /**
     * @brief Write TCON register 1
     * @param value TCON value (9-bit, 0x000-0x1FF)
     * @return mcp44xx::expected<void> - error on failure
     */
    mcp44xx::expected<void> write_tcon1(uint16_t value);

    /**
     * @brief Read TCON register 1
     * @param value Pointer to store TCON value
     * @return mcp44xx::expected<void> - error on failure
     */
    mcp44xx::expected<void> read_tcon1(uint16_t& value);

    /**
     * @brief Write to EEPROM slot
     * @param slot EEPROM slot index (0-4)
     * @param value Data value (9-bit, 0x000-0x1FF)
     * @return mcp44xx::expected<void> - error on failure
     */
    mcp44xx::expected<void> write_eeprom(uint8_t slot, uint16_t value);

    /**
     * @brief Read from EEPROM slot
     * @param slot EEPROM slot index (0-4)
     * @param value Pointer to store data value
     * @return mcp44xx::expected<void> - error on failure
     */
    mcp44xx::expected<void> read_eeprom(uint8_t slot, uint16_t& value);

    /**
     * @brief Wait for EEPROM write to complete
     * @param timeout_ms Timeout in milliseconds
     * @return mcp44xx::expected<void> - error on timeout
     */
    mcp44xx::expected<void> wait_eeprom_ready(uint32_t timeout_ms = 15);

    /**
     * @brief Get 7-bit I2C address
     */
    uint8_t get_address() const { return address7_; }

    /**
     * @brief Get resolution
     */
    Resolution get_resolution() const { return resolution_; }

    /**
     * @brief Check if driver is initialized
     */
    bool is_initialized() const { return initialized_; }

private:
    I2C_HandleTypeDef* hi2c_;
    uint8_t address7_;
    Resolution resolution_;
    uint32_t timeout_ms_;
    bool initialized_;

    // Register addresses
    static constexpr uint8_t REG_W0_V  = 0x00;
    static constexpr uint8_t REG_W1_V  = 0x01;
    static constexpr uint8_t REG_TCON0 = 0x04;
    static constexpr uint8_t REG_STATUS = 0x05;
    static constexpr uint8_t REG_W2_V  = 0x06;
    static constexpr uint8_t REG_W3_V  = 0x07;
    static constexpr uint8_t REG_TCON1 = 0x0A;

    static constexpr uint8_t REG_W0_NV = 0x02;
    static constexpr uint8_t REG_W1_NV = 0x03;
    static constexpr uint8_t REG_W2_NV = 0x08;
    static constexpr uint8_t REG_W3_NV = 0x09;

    static constexpr uint8_t REG_EE0 = 0x0B;

    // Command byte: AD3:AD0 | C1:C0 | D9 | D8
    static constexpr uint8_t CMD_WRITE = 0x00;
    static constexpr uint8_t CMD_INC   = 0x04;
    static constexpr uint8_t CMD_DEC   = 0x08;
    static constexpr uint8_t CMD_READ  = 0x0C;

    // Status register bits
    static constexpr uint16_t STATUS_EEWA = (1u << 4);
    static constexpr uint16_t STATUS_WP   = (1u << 0);
    static constexpr uint16_t STATUS_WL0  = (1u << 2);
    static constexpr uint16_t STATUS_WL1  = (1u << 3);
    static constexpr uint16_t STATUS_WL2  = (1u << 5);
    static constexpr uint16_t STATUS_WL3  = (1u << 6);

    /**
     * @brief Get volatile wiper register address
     */
    uint8_t volatile_wiper_reg(Wiper wiper) const;

    /**
     * @brief Get non-volatile wiper register address
     */
    uint8_t nonvolatile_wiper_reg(Wiper wiper) const;

    /**
     * @brief Write a register value
     */
    mcp44xx::expected<void> write_register(uint8_t reg, uint16_t value);

    /**
     * @brief Read a register value
     */
    mcp44xx::expected<void> read_register(uint8_t reg, uint16_t& value);

    /**
     * @brief Send a command-only transaction
     */
    mcp44xx::expected<void> command_only(uint8_t reg, uint8_t command);

    /**
     * @brief Convert HAL status to expected error code
     */
    static std::error_code convert_hal_status(HAL_StatusTypeDef status);

    /**
     * @brief Validate wiper number
     */
    static bool valid_wiper(Wiper wiper);
};

} // namespace mcp44xx

#endif // __MCP44XX_HPP
