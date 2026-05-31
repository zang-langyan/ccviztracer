cmake_minimum_required(VERSION 3.20)
project(cctrace)

if (CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(GNU 1)
elseif (CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(LLVM 1)
endif ()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

include_directories(${PROJECT_SOURCE_DIR}/src)

# ====================================================================================
# [[ LLVM ]]
if (LLVM)
    find_package(LLVM REQUIRED CONFIG)
    message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
    message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
    message(STATUS "LLVM include: ${LLVM_INCLUDE_DIRS}")
    message(STATUS "LLVM library: ${LLVM_LIBRARY_DIRS}")
    message(STATUS "LLVM tools binary: ${LLVM_TOOLS_BINARY_DIR}")
    include_directories(${LLVM_INCLUDE_DIRS})

    # [[ LLVM Pass ]]
    add_library(cctracer_pass MODULE ${PROJECT_SOURCE_DIR}/src/llvm/CCTracerPass.cpp)
    target_link_libraries(cctracer_pass LLVM)

    set_target_properties(cctracer_pass PROPERTIES
        PREFIX ""
        SUFFIX ".dylib"
    )
endif (LLVM)

if (GNU)
    # PLUGIN_INCLUDE=$(gcc -print-file-name=plugin)/include
    execute_process(
        COMMAND ${CMAKE_C_COMPILER} -print-file-name=plugin
        OUTPUT_VARIABLE GCC_PLUGIN_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
        COMMAND_ERROR_IS_FATAL ANY
    )
    set(GCC_PLUGIN_INCLUDE_DIR "${GCC_PLUGIN_DIR}/include")
    message(STATUS "GCC plugin include directory: ${GCC_PLUGIN_INCLUDE_DIR}")

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GMP REQUIRED gmp)
    message(STATUS "GMP include directory: ${GMP_INCLUDE_DIRS}")
    message(STATUS "GMP library: ${GMP_LIBRARIES}")

    add_library(cctracer_pass MODULE ${PROJECT_SOURCE_DIR}/src/gnu/gCCTracerPass.cpp)
    target_include_directories(cctracer_pass PRIVATE 
        ${GCC_PLUGIN_INCLUDE_DIR}
        ${GMP_INCLUDE_DIRS}
    )
    if (APPLE)
        target_link_options(cctracer_pass PRIVATE -Wl,-undefined,dynamic_lookup)
    endif (APPLE)
    target_link_directories(cctracer_pass PRIVATE ${GMP_LIBRARY_DIRS})
    target_link_libraries(cctracer_pass PRIVATE ${GMP_LIBRARIES})
    target_compile_options(cctracer_pass PRIVATE -fPIC -fno-rtti)

    set_target_properties(cctracer_pass PROPERTIES
        PREFIX ""
        SUFFIX ".dylib"
    )
endif (GNU)
# ====================================================================================

# [[ Runtime ]]
add_library(cctracer STATIC ${PROJECT_SOURCE_DIR}/src/runtime/cctracer.cpp)

# [[ Example ]]
set(EXAMPLE_SRC_DIR ${PROJECT_SOURCE_DIR}/example)
add_executable(example ${EXAMPLE_SRC_DIR}/example.cpp)
if (GNU) 
    set(PLUGIN_OPTION "-fplugin=${CMAKE_CURRENT_BINARY_DIR}/cctracer_pass.dylib")
elseif (LLVM) 
    set(PLUGIN_OPTION "-fpass-plugin=${CMAKE_CURRENT_BINARY_DIR}/cctracer_pass.dylib")
endif ()
target_compile_options(example PRIVATE 
    -g
    -O3
    ${PLUGIN_OPTION}   
)
target_link_libraries(example cctracer)

add_executable(multithread ${EXAMPLE_SRC_DIR}/multithread.cpp)
target_compile_options(multithread PRIVATE 
    -g
    -O3
    ${PLUGIN_OPTION}   
)
target_link_libraries(multithread cctracer)

option(BUILD_TEST "Build cctracer tests" ON)
if (BUILD_TEST)
    message("building cctracer tests")
    add_subdirectory(test)
endif()
