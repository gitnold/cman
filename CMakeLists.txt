cmake_minimum_required(VERSION 3.10)

project(cman)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Disable extensions. Don't know if this might be necessary
# set(CMAKE_CXX_EXTENSIONS OFF)
#
add_executable(cman src/*.cpp src/*.c)
set_target_properties(cman PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)

target_compile_options(cman PRIVATE -Wall -Wextra)
