# =============================================================================
# MCP44XX Digital Potentiometer Driver CMake Library
# =============================================================================
# This library provides an I2C driver for the Microchip MCP444x/446x series
# of digital potentiometers for use with STM32 microcontrollers using the HAL.
#
# Usage in parent project:
#   add_subdirectory(mcp44xx_stm32_driver)
#   target_link_libraries(your_target PRIVATE mcp44xx)
# =============================================================================

cmake_minimum_required(VERSION 3.15)

# =============================================================================
# Configuration
# =============================================================================

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(mcp44xx
    LANGUAGES CXX C
    VERSION 1.0.0
    DESCRIPTION "MCP44XX Digital Potentiometer Driver for STM32 HAL"
)

# =============================================================================
# Configuration Options
# =============================================================================

option(MCP44XX_ENABLE_LOGGING "Enable verbose logging" OFF)
option(MCP44XX_ENABLE_ASSERTS "Enable debug assertions" OFF)
option(MCP44XX_BUILD_TESTS "Build STM32 test application" OFF)
option(MCP44XX_BUILD_UNIT_TESTS "Build unit tests with mock HAL" ON)
option(MCP44XX_INSTALL "Install MCP44XX driver" OFF)

# Integrated mode: Use project's CMSIS and HAL paths
option(MCP44XX_INTEGRATED_MODE "Use project-provided CMSIS/HAL paths" OFF)

# Integrated mode settings (provided by parent project)
set(MCP44XX_CMSIS_INC_DIR "" CACHE STRING "Path to CMSIS include directory")
set(MCP44XX_HAL_INC_DIR "" CACHE PATH "Path to HAL driver include directory")
set(MCP44XX_MCU "" CACHE STRING "MCU name (e.g., STM32G474xx)")

# Standalone mode settings
set(STM32_DEVICE STM32G474 CACHE STRING "STM32 device")
set(STM32_CUBE_PATH "" CACHE PATH "Path to STM32CubeG4 Drivers directory")

# =============================================================================
# Find STM32 HAL
# =============================================================================

if(MCP44XX_INTEGRATED_MODE)
    if(NOT MCP44XX_CMSIS_INC_DIR OR NOT MCP44XX_HAL_INC_DIR OR NOT MCP44XX_MCU)
        message(FATAL_ERROR "MCP44XX_INTEGRATED_MODE requires MCP44XX_CMSIS_INC_DIR, MCP44XX_HAL_INC_DIR, and MCP44XX_MCU to be set")
    endif()

    message(STATUS "MCP44XX: Building in integrated mode with project-provided headers")
    # Only create stm32_hal if it doesn't already exist (from another driver or parent project)
    if(NOT TARGET stm32_hal)
        add_library(stm32_hal INTERFACE)
        target_include_directories(stm32_hal INTERFACE ${MCP44XX_CMSIS_INC_DIR} ${MCP44XX_HAL_INC_DIR} ${MCP44XX_BOARD_INC_DIR})
        target_compile_definitions(stm32_hal INTERFACE ${MCP44XX_MCU} USE_HAL_DRIVER)
    endif()
else()
    if(NOT STM32_CUBE_PATH)
        message(FATAL_ERROR "STM32_CUBE_PATH must be set. Example: -DSTM32_CUBE_PATH=/path/to/STM32CubeG4/Drivers")
    endif()

    if(NOT EXISTS ${STM32_CUBE_PATH})
        message(FATAL_ERROR "STM32_CUBE_PATH does not exist: ${STM32_CUBE_PATH}")
    endif()

    message(STATUS "MCP44XX: Building in standalone mode with STM32Cube path")

    enable_language(C)

    add_library(stm32_hal STATIC)
    target_sources(stm32_hal
        PRIVATE
            ${STM32_CUBE_PATH}/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal.c
            ${STM32_CUBE_PATH}/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_i2c.c
            ${STM32_CUBE_PATH}/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_gpio.c
            ${STM32_CUBE_PATH}/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_rcc.c
            ${STM32_CUBE_PATH}/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_cortex.c
    )

    target_include_directories(stm32_hal
        PUBLIC
            ${STM32_CUBE_PATH}/STM32G4xx_HAL_Driver/Inc
            ${STM32_CUBE_PATH}/CMSIS/Device/ST/STM32G4xx/Include
    )

    target_compile_definitions(stm32_hal
        PUBLIC
            ${STM32_DEVICE}
            USE_HAL_DRIVER
            HSE_VALUE=16000000
            HSI_VALUE=16000000
    )

    target_compile_options(stm32_hal
        PRIVATE
            -mcpu=cortex-m4
            -mthumb
            -mfloat-abi=hard
            -mfpu=fpv4-sp-d16
            -O2
            -Wall
            -Wextra
    )
endif()

# =============================================================================
# Library Definition
# =============================================================================

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/mcp44xx.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
)

if(MCP44XX_INTEGRATED_MODE)
    target_include_directories(${PROJECT_NAME}
        PRIVATE
            ${MCP44XX_CMSIS_INC_DIR}
            ${MCP44XX_HAL_INC_DIR}
            ${MCP44XX_BOARD_INC_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
    )
else()
    target_include_directories(${PROJECT_NAME}
        PRIVATE
            ${STM32_CUBE_PATH}/STM32G4xx_HAL_Driver/Inc
            ${STM32_CUBE_PATH}/CMSIS/Include
            ${STM32_CUBE_PATH}/CMSIS/Device/ST/STM32G4xx/Include
    )
endif()

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        stm32_hal
)

# =============================================================================
# Requirements
# =============================================================================

set_target_properties(${PROJECT_NAME}
    PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)

# Exception handling required for expected type
target_compile_options(${PROJECT_NAME}
    PRIVATE
        -fexceptions
)

# =============================================================================
# Compile Definitions
# =============================================================================

target_compile_definitions(${PROJECT_NAME}
    PUBLIC
        MCP44XX_I2C_ADDRESS=0x2C
        MCP44XX_TIMEOUT_MS=100
)

# =============================================================================
# Optional Features
# =============================================================================

if(MCP44XX_ENABLE_LOGGING)
    target_compile_definitions(${PROJECT_NAME}
        PUBLIC
            MCP44XX_ENABLE_LOGGING=1
    )
endif()

if(MCP44XX_ENABLE_ASSERTS)
    target_compile_definitions(${PROJECT_NAME}
        PUBLIC
            MCP44XX_ENABLE_ASSERTS=1
    )
endif()

# =============================================================================
# Test Application (STM32 - requires STM32CubeG4)
# =============================================================================

if(MCP44XX_BUILD_TESTS)
    message(STATUS "STM32 test application requires STM32CubeG4 path")
    message(STATUS "Enable with: -DMCP44XX_BUILD_TESTS=ON -DSTM32_CUBE_PATH=/path/to/STM32CubeG4/Drivers")
endif()

# =============================================================================
# Unit Tests with Mocks (Host-based testing)
# =============================================================================

if(MCP44XX_BUILD_UNIT_TESTS)
    enable_testing()

    # Since we cannot change compilers after project() is defined,
    # we create a custom target that builds the test with the host compiler directly

    set(TEST_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_mcp44xx.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/mock_mcp44xx.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/mcp44xx.cpp
    )

    # Determine include paths based on mode
    if(MCP44XX_INTEGRATED_MODE)
        set(HAL_INCLUDE_PATHS "")
        foreach(path IN LISTS MCP44XX_CMSIS_INC_DIR)
            list(APPEND HAL_INCLUDE_PATHS -I${path})
        endforeach()
        foreach(path IN LISTS MCP44XX_HAL_INC_DIR)
            list(APPEND HAL_INCLUDE_PATHS -I${path})
        endforeach()
    else()
        set(HAL_INCLUDE_PATHS
            -I${STM32_CUBE_PATH}/STM32G4xx_HAL_Driver/Inc
            -I${STM32_CUBE_PATH}/CMSIS/Include
            -I${STM32_CUBE_PATH}/CMSIS/Device/ST/STM32G4xx/Include
        )
    endif()

    # Create custom target that runs g++ directly
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/test_mcp44xx.elf
        COMMAND /usr/bin/g++ -std=c++17 -fexceptions
            -I${CMAKE_CURRENT_SOURCE_DIR}
            -I${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
            -I${CMAKE_CURRENT_SOURCE_DIR}/tests
            ${HAL_INCLUDE_PATHS}
            -DTESTING
            -DHAL_MODULE_ENABLED
            -D${STM32_DEVICE}
            -o ${CMAKE_CURRENT_BINARY_DIR}/test_mcp44xx.elf
            ${TEST_SOURCES}
        DEPENDS ${TEST_SOURCES}
        COMMENT "Building MCP44XX unit test with host compiler"
    )

    add_custom_target(test_mcp44xx
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/test_mcp44xx.elf
    )

    # Add test
    add_test(NAME test_mcp44xx COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_mcp44xx.elf)

    message(STATUS "Unit tests enabled - build with 'make test_mcp44xx'")
endif()

# =============================================================================
# Installation
# =============================================================================

if(MCP44XX_INSTALL)
    include(GNUInstallDirs)

    install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}-targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(EXPORT ${PROJECT_NAME}-targets
        FILE
            ${PROJECT_NAME}-targets.cmake
        NAMESPACE
            mcp44xx::
        DESTINATION
            lib/cmake/mcp44xx
    )

    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
        DESTINATION include
    )

    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/README.md
        DESTINATION share/doc/mcp44xx
    )
endif()

# =============================================================================
# Summary
# =============================================================================

message(STATUS "")
message(STATUS "MCP44XX Digital Potentiometer Driver Configuration:")
message(STATUS "  Version:           ${PROJECT_VERSION}")
message(STATUS "  Device:            ${STM32_DEVICE}")
message(STATUS "  Integrated Mode:   ${MCP44XX_INTEGRATED_MODE}")
if(MCP44XX_INTEGRATED_MODE)
    message(STATUS "  CMSIS Include:     ${MCP44XX_CMSIS_INC_DIR}")
    message(STATUS "  HAL Include:       ${MCP44XX_HAL_INC_DIR}")
else()
    message(STATUS "  Cube Path:         ${STM32_CUBE_PATH}")
endif()
message(STATUS "  Logging:           ${MCP44XX_ENABLE_LOGGING}")
message(STATUS "  Assertions:        ${MCP44XX_ENABLE_ASSERTS}")
message(STATUS "  Build Tests:       ${MCP44XX_BUILD_TESTS}")
message(STATUS "  Build Unit Tests:  ${MCP44XX_BUILD_UNIT_TESTS}")
message(STATUS "  Install:           ${MCP44XX_INSTALL}")
message(STATUS "")
message(STATUS "Available targets:")
message(STATUS "  mcp44xx           - Build MCP44XX library")
if(MCP44XX_BUILD_TESTS)
    message(STATUS "  mcp44xx_test      - Build test application (requires STM32CubeG4)")
endif()
if(MCP44XX_BUILD_UNIT_TESTS)
    message(STATUS "  test_mcp44xx      - Build unit tests (host)")
endif()
message(STATUS "")
message(STATUS "To build (integrated mode):")
message(STATUS "  add_subdirectory(mcp44xx_stm32_driver)")
message(STATUS "  target_link_libraries(your_target PRIVATE mcp44xx)")
message(STATUS "")
message(STATUS "To build (standalone mode):")
message(STATUS "  mkdir build && cd build")
message(STATUS "  cmake .. -DSTM32_DEVICE=${STM32_DEVICE} -DSTM32_CUBE_PATH=<path-to-STM32CubeG4>")
message(STATUS "  make")
if(MCP44XX_BUILD_TESTS)
    message(STATUS "  make mcp44xx_test - Build STM32 test application")
endif()
if(MCP44XX_BUILD_UNIT_TESTS)
    message(STATUS "  make test_mcp44xx - Build unit tests")
    message(STATUS "  ctest            - Run unit tests")
endif()
message(STATUS "")
