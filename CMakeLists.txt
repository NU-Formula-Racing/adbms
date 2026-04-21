cmake_minimum_required(VERSION 3.15)

project(adbms_test C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(adbms_test
    adbms_code/Src/test.c
    adbms_code/Src/adbms_driver_v2.c
    adbms_code/Src/adbms_interface_2950.c
    adbms_code/Src/adbms_interface_6830.c
    adbms_code/Src/adbms_interface_v2.c
)

target_include_directories(adbms_test PRIVATE
    adbms_code/Inc
)