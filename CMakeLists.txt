cmake_minimum_required(VERSION 3.16)
project(HelloWorld LANGUAGES CXX)

add_executable(hello_world main.cpp)
set_target_properties(hello_world PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
