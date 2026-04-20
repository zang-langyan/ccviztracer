cmake_minimum_required(VERSION 3.20)
project(cctracer_test)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# [[ TEST ]]
add_executable(test_config ${PROJECT_SOURCE_DIR}/configparser.cpp)
target_include_directories(test_config PRIVATE ${PROJECT_SOURCE_DIR}/../src/util)

