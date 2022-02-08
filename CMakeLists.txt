cmake_minimum_required(VERSION 3.14)
set(CMAKE_EXPORT_COMPILE_COMMANDS)
set(CMAKE_CXX_STANDARD 17)

project("scc")

add_executable(
  ${PROJECT_NAME}
  main.cc
)
