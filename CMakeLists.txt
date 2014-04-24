cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "-g -std=c++0x")

# A convenience variable:
set(LLVM_ROOT "../llvm_install" CACHE PATH "Root of LLVM install.")

# A bit of a sanity check:
if( NOT EXISTS ${LLVM_ROOT}/include/llvm )
message(FATAL_ERROR "LLVM_ROOT (${LLVM_ROOT}) is not a valid LLVM install")
endif()

# We incorporate the CMake features provided by LLVM:
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${LLVM_ROOT}/share/llvm/cmake")
set(LLVM_INCLUDE_DIRS "/pool/users/pc424/llvm/include/llvm")
include(LLVMConfig)


add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

set(LLVM_RUNTIME_OUTPUT_INTDIR ".")
set(LLVM_LIBRARY_OUTPUT_INTDIR ".")

# Define add_llvm_* macro's.
include(AddLLVM)

add_subdirectory(Basic)
add_subdirectory(PointerAnalysis)
