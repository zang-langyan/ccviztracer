cmake_minimum_required(VERSION 3.20)
project(cctrace)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

# [[ LLVM ]]
# TODO: Use find_package(LLVM) instead of hardcoding paths
set(LLVM_INCLUDE_DIRS "/opt/homebrew/opt/llvm/include")
set(LLVM_LIB_DIRS "/opt/homebrew/opt/llvm/lib")
set(SRC_DIR ${PROJECT_SOURCE_DIR}/src/)
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${SRC_DIR})
link_directories(${LLVM_LIB_DIRS})


# [[ Perfetto SDK ]]
find_package(Threads)
include_directories(${PROJECT_SOURCE_DIR}/extern/perfetto_sdk)
add_subdirectory(${PROJECT_SOURCE_DIR}/extern/perfetto_sdk)


# [[ LLVM Pass ]]
add_library(cctracer_pass MODULE ${SRC_DIR}/llvm/CCTracerPass.cpp)
target_link_libraries(cctracer_pass LLVM)

set_target_properties(cctracer_pass PROPERTIES
    PREFIX ""
    SUFFIX ".dylib"
)

# [[ Runtime ]]
add_library(cctracer STATIC src/runtime/cctracer.cpp)
target_link_libraries(cctracer perfetto ${CMAKE_THREAD_LIBS_INIT})
if (WIN32)
  target_link_libraries(cctracer ws2_32)
endif (WIN32)
# Enable standards-compliant mode when using the Visual Studio compiler.
if (MSVC)
  target_compile_options(cctracer PRIVATE "/permissive-")
endif (MSVC)

# [[ Example ]]
set(EXAMPLE_SRC_DIR ${PROJECT_SOURCE_DIR}/example/example.cpp)
add_executable(example ${EXAMPLE_SRC_DIR})
target_compile_options(example PRIVATE 
  -g
  -O3
  -fpass-plugin=${CMAKE_CURRENT_BINARY_DIR}/cctracer_pass.dylib    
)
target_link_libraries(example cctracer)

option(BUILD_TEST "Build cctracer tests" ON)
if (BUILD_TEST)
  message("building cctracer tests")
  add_subdirectory(test)
endif()