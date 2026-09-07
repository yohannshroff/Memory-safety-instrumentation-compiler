cmake_minimum_required(VERSION 3.16)
project(memsafety LANGUAGES C CXX)

#
# memsafety - out-of-tree LLVM instrumentation pass + C runtime + unit tests.
#
#   cmake -B build [-DLLVM_DIR=/path/to/llvm/lib/cmake/llvm]
#   cmake --build build
#   ctest --test-dir build --output-on-failure
#
# On macOS with Homebrew LLVM:
#   cmake -B build -DLLVM_DIR="$(brew --prefix llvm)/lib/cmake/llvm"
#

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "" FORCE)
endif()

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)

separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND "${LLVM_DEFINITIONS}")
add_definitions(${LLVM_DEFINITIONS_LIST})

enable_testing()

add_subdirectory(runtime)
add_subdirectory(src)
add_subdirectory(tests)
