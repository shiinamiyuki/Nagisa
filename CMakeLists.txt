cmake_minimum_required(VERSION 3.12)
project(AkariCompute)
set(CMAKE_CXX_STANDARD 17)


IF(WIN32)
    set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "C:/Program Files (x86)/LLVM/lib/cmake")
ELSE()
   set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "/usr/lib64/cmake/")
ENDIF()
set(LLVM_ROOT "" CACHE STRING "")
if(LLVM_ROOT STREQUAL  "")
    find_package(LLVM 10.0)
else()
    find_package(LLVM PATHS ${LLVM_ROOT})
endif()
if(NOT LLVM_FOUND)
    message(FATAL_ERROR "LLVM not found")
else()
    message(STATUS "Found LLVM ${LLVM_VERSION}")
    message(STATUS "LLVM_INCLUDE_DIR " ${LLVM_INCLUDE_DIR})
    message(STATUS "LLVM_LIBRARY_DIRS    " ${LLVM_LIBRARY_DIRS})
endif()

file(GLOB NAGISA_SRC src/nagisa-rt/*.* src/nagisa-rt/*/*.*)

add_library(NagisaRT SHARED ${NAGISA_SRC})
include_directories(include/)
target_include_directories(NagisaRT PUBLIC external/magic_enum/include)
target_include_directories(NagisaRT PUBLIC ${LLVM_INCLUDE_DIR})
target_include_directories(NagisaRT PUBLIC external/boost)
target_link_directories(NagisaRT PUBLIC ${LLVM_LIBRARY_DIRS})
target_link_libraries(NagisaRT
        LLVMX86Disassembler
        LLVMX86AsmParser
        # LLVMX86AsmPrinter
        LLVMX86CodeGen

        LLVMSelectionDAG

        LLVMAsmPrinter
        LLVMMCParser
        LLVMX86Info

        LLVMMCJIT
        LLVMInterpreter
        LLVMExecutionEngine

        LLVMCodeGen
        LLVMScalarOpts
        LLVMTransformUtils

        LLVMipo
        LLVMAnalysis
        LLVMTarget
        LLVMMC

        LLVMCore
        LLVMSupport
        )
add_executable(simple src/examples/simple-expression.cpp)
target_link_libraries(simple NagisaRT)