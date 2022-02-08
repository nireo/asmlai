cmake_minimum_required(VERSION 3.14)
project(asmlai)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_executable(
  asmlai
  src/codegen_x64.cc
  src/compiler.cc
  src/lexer.cc
  src/parser.cc
  src/types.cc
  src/main.cc
  )
