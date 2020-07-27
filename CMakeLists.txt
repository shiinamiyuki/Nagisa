cmake_minimum_required(VERSION 3.12)
project(AkariCompute)
set(CMAKE_CXX_STANDARD 17)


file(GLOB NAGISA_SRC src/*.* src/*/*.*)
find_package(OpenCL)
add_library(NagisaRT ${NAGISA_SRC})
include_directories(include/)
target_include_directories(NagisaRT PUBLIC ${OpenCL_INCLUDE_DIRS})

target_include_directories(NagisaRT PUBLIC external/boost)
target_link_libraries(NagisaRT ${OpenCL_LIBRARIES})
add_executable(simple examples/simple.cpp)
target_link_libraries(simple NagisaRT)