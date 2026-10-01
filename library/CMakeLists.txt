# =============================================================================
# MCP44XX Digital Potentiometer Driver - Library Integration CMakeLists.txt
# =============================================================================
# Usage in parent project:
#   add_subdirectory(path/to/mcp44xx_stm32_driver/library)
#   target_link_libraries(your_target PRIVATE mcp44xx)
# =============================================================================

cmake_minimum_required(VERSION 3.15)

# =============================================================================
# Library Definition
# =============================================================================

add_library(mcp44xx STATIC)

# Add source files
target_sources(mcp44xx
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Src/mcp44xx.cpp
)

# Add include directories
# Core/Inc is at ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Inc
target_include_directories(mcp44xx
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../Core/Inc>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Inc
)

# Dependencies (HAL library must be provided by parent project)
target_link_libraries(mcp44xx
    PUBLIC
        $<TARGET_NAME_IF_EXISTS:stm32_hal>
)

# =============================================================================
# Requirements
# =============================================================================

set_target_properties(mcp44xx
    PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)

# =============================================================================
# Compile Definitions
# =============================================================================

target_compile_definitions(mcp44xx
    PUBLIC
        MCP44XX_I2C_ADDRESS=0x2C
        MCP44XX_TIMEOUT_MS=100
)
