cmake_minimum_required(VERSION 3.12.0)
project(llvm-xpass)

set(CLANG_VERSION 14)

# Set this to a valid LLVM installation dir
set(LT_LLVM_INSTALL_DIR "/usr/lib/llvm-${CLANG_VERSION}" CACHE PATH "LLVM installation directory")

# Add the location of LLVMConfig.cmake to CMake search paths
list(APPEND CMAKE_PREFIX_PATH "${LT_LLVM_INSTALL_DIR}/lib/cmake/llvm/")

###############################################################################

if(NOT (CMAKE_C_COMPILER MATCHES "clang" AND
      CMAKE_C_COMPILER_VERSION VERSION_EQUAL ${CLANG_VERSION}))
  message(FATAL_ERROR
    "Please specify clang version ${CLANG_VERSION} as a C compiler. (e.g. CC=clang-${CLANG_VERSION} cmake ..)")
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Type of build" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

enable_testing()

set(LT_LLVM_INCLUDE_DIR "${LT_LLVM_INSTALL_DIR}/include/llvm")
if(NOT EXISTS "${LT_LLVM_INCLUDE_DIR}")
message(FATAL_ERROR
  " LT_LLVM_INSTALL_DIR (${LT_LLVM_INCLUDE_DIR}) is invalid.")
endif()

set(LT_LLVM_CMAKE_FILE "${LT_LLVM_INSTALL_DIR}/lib/cmake/llvm/LLVMConfig.cmake")
if(NOT EXISTS "${LT_LLVM_CMAKE_FILE}")
message(FATAL_ERROR
    " LT_LLVM_CMAKE_FILE (${LT_LLVM_CMAKE_FILE}) is invalid.")
endif()

find_package(LLVM ${CLANG_VERSION}.0.0 REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LT_LLVM_INSTALL_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})
add_definitions(${LLVM_DEFINITIONS})

set(CMAKE_CXX_STANDARD 14 CACHE STRING "")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-fvisibility-inlines-hidden" SUPPORTS_FVISIBILITY_INLINES_HIDDEN_FLAG)
if (${SUPPORTS_FVISIBILITY_INLINES_HIDDEN_FLAG} EQUAL "1")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility-inlines-hidden")
endif()

#

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")

find_program(FILECHECK_COMMAND NAMES FileCheck-${CLANG_VERSION} FileCheck)
find_program(OPT_COMMAND NAMES opt-${CLANG_VERSION} opt)

##

set(TARGET_MATHPEEPHOLE "MathPeephole")

add_subdirectory(MathPeephole)
add_subdirectory(tester)
