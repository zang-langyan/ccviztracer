cmake_minimum_required(VERSION 3.20)
project(cctrace)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

# [[ LLVM ]]
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "LLVM include: ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM library: ${LLVM_LIBRARY_DIRS}")
message(STATUS "LLVM tools binary: ${LLVM_TOOLS_BINARY_DIR}")
include_directories(${LLVM_INCLUDE_DIRS})


# [[ Perfetto SDK ]]
find_package(Threads)
include_directories(${PROJECT_SOURCE_DIR}/extern/perfetto_sdk)
add_subdirectory(${PROJECT_SOURCE_DIR}/extern/perfetto_sdk)


# [[ LLVM Pass ]]
include_directories(${PROJECT_SOURCE_DIR}/src)
add_library(cctracer_pass MODULE ${PROJECT_SOURCE_DIR}/src/llvm/CCTracerPass.cpp)
target_link_libraries(cctracer_pass LLVM)

set_target_properties(cctracer_pass PROPERTIES
    PREFIX ""
    SUFFIX ".dylib"
)

# [[ Runtime ]]
add_library(cctracer STATIC ${PROJECT_SOURCE_DIR}/src/runtime/cctracer.cpp)
target_link_libraries(cctracer perfetto ${CMAKE_THREAD_LIBS_INIT})
if (WIN32)
  target_link_libraries(cctracer ws2_32)
endif (WIN32)
# Enable standards-compliant mode when using the Visual Studio compiler.
if (MSVC)
  target_compile_options(cctracer PRIVATE "/permissive-")
endif (MSVC)

# [[ Example ]]
set(EXAMPLE_SRC_DIR ${PROJECT_SOURCE_DIR}/example)
add_executable(example ${EXAMPLE_SRC_DIR}/example.cpp)
target_compile_options(example PRIVATE 
  -g
  -O3
  -fpass-plugin=${CMAKE_CURRENT_BINARY_DIR}/cctracer_pass.dylib    
)
target_link_libraries(example cctracer)

add_executable(multithread ${EXAMPLE_SRC_DIR}/multithread.cpp)
target_compile_options(multithread PRIVATE 
  -g
  -O3
  -fpass-plugin=${CMAKE_CURRENT_BINARY_DIR}/cctracer_pass.dylib    
)
target_link_libraries(multithread cctracer)

option(BUILD_TEST "Build cctracer tests" ON)
if (BUILD_TEST)
  message("building cctracer tests")
  add_subdirectory(test)
endif()
