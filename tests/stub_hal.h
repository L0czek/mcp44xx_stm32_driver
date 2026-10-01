/**
 * @file stub_hal.h
 * @brief Stub HAL headers for unit testing without STM32 toolchain
 */

#ifndef __STUB_HAL_H
#define __STUB_HAL_H

#include <cstdint>
#include <cstring>

// HAL status definitions
typedef enum {
    HAL_OK = 0,
    HAL_ERROR = 1,
    HAL_BUSY = 2,
    HAL_TIMEOUT = 3
} HAL_StatusTypeDef;

// I2C handle structure (simplified for testing)
// UserData is used by MockMcp44xx to intercept I2C calls
struct I2C_HandleTypeDef {
    void* UserData;
};

// I2C memory address size
typedef enum {
    I2C_MEMADD_SIZE_8BIT = 1,
    I2C_MEMADD_SIZE_16BIT = 2
} I2C_MEMADD_SIZETypeDef;

// HAL I2C function declarations (mocked in mock_mcp44xx.cpp)
HAL_StatusTypeDef HAL_I2C_IsDeviceReady(I2C_HandleTypeDef* hi2c, uint16_t DevAddress,
                                         uint32_t Retry, uint32_t Timeout);
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef* hi2c, uint16_t DevAddress,
                                     uint16_t MemAddress, uint16_t MemAddSize,
                                     uint8_t* pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef* hi2c, uint16_t DevAddress,
                                    uint16_t MemAddress, uint16_t MemAddSize,
                                    uint8_t* pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef* hi2c, uint16_t DevAddress,
                                          uint8_t* pData, uint16_t Size, uint32_t Timeout);

// Generic HAL functions
void HAL_Delay(uint32_t Delay);
uint32_t HAL_GetTick(void);
void HAL_ResetTick(void);

#endif // __STUB_HAL_H
