cmake_minimum_required(VERSION 3.24)
project(odf_package_consumer LANGUAGES CXX)

find_package(odf 0.1 CONFIG REQUIRED)
add_executable(odf_package_consumer main.cpp)
target_link_libraries(odf_package_consumer PRIVATE
    odf::core odf::torch_core odf::paddle_core)

