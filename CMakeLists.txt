cmake_minimum_required(VERSION 3.10)
project(Example)

set(CMAKE_CXX_STANDARD 17)

add_executable(Example example.cpp)
target_link_libraries(Example sqlite3)