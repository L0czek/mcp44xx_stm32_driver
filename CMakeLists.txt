cmake_minimum_required(VERSION 3.10)

project(mcp44xx_stm32_driver_tests
    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add executable for tests
add_executable(mcp44xx_tests
    tests/test_mcp44xx.cpp
    tests/mock_mcp44xx.cpp
    Core/Src/mcp44xx.cpp
)

# Add include directories
target_include_directories(mcp44xx_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

# Define HAL macros for testing (stub implementations)
target_compile_definitions(mcp44xx_tests PRIVATE
    TESTING
    HAL_MODULE_ENABLED
    STM32G4xx
)

# Link standard libraries
target_link_libraries(mcp44xx_tests
    stdc++
)

# Add test target
add_test(NAME mcp44xx_tests COMMAND mcp44xx_tests)
