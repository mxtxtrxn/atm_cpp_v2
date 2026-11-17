cmake_minimum_required(VERSION 3.16)
project(atm_cpp_v2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(ATM_CORE_SOURCES
    src/money.cpp
    src/account.cpp
    src/transaction.cpp
    src/auth.cpp
    src/storage.cpp
    src/bank.cpp
    src/ui.cpp
    src/cli.cpp
)

add_library(atm_core STATIC ${ATM_CORE_SOURCES})
target_include_directories(atm_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(MSVC)
    target_compile_options(atm_core PRIVATE /W4)
else()
    target_compile_options(atm_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(atm src/main.cpp)
target_link_libraries(atm PRIVATE atm_core)

if(MINGW)
    target_link_options(atm PRIVATE -static)
endif()

option(ATM_BUILD_TESTS "Build the test suite" ON)

if(ATM_BUILD_TESTS)
    enable_testing()

    add_executable(atm_tests
        tests/test_main.cpp
        tests/test_money.cpp
        tests/test_account.cpp
        tests/test_auth.cpp
        tests/test_bank.cpp
        tests/test_storage.cpp
        tests/test_cli.cpp
    )
    target_link_libraries(atm_tests PRIVATE atm_core)
    target_include_directories(atm_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)

    if(MSVC)
        target_compile_options(atm_tests PRIVATE /W4)
    else()
        target_compile_options(atm_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    if(MINGW)
        target_link_options(atm_tests PRIVATE -static)
    endif()

    add_test(NAME atm_tests COMMAND atm_tests)
endif()
