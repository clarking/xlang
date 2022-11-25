
cmake_minimum_required(VERSION 3.19)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall")
project(xlang)

set(CMAKE_BINARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build")

add_executable(xlang
        src/analyze.cpp
        src/convert.cpp
        src/insn.cpp
        src/lex.cpp
        src/main.cpp
        src/murmurhash3.cpp
        src/optimize.cpp
        src/parser.cpp
        src/print.cpp
        src/regs.cpp
        src/symtab.cpp
        src/tree.cpp
        src/gen.cpp
        src/compiler.cpp)

target_include_directories(xlang PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)