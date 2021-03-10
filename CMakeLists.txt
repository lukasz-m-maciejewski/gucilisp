cmake_minimum_required(VERSION 3.16)
project(gucilisp)

add_executable(gucilisp main.cpp)
target_link_libraries(gucilisp PUBLIC readline)
target_compile_features(gucilisp PUBLIC cxx_std_20)
target_include_directories(gucilisp PUBLIC ${CMAKE_SOURCE_DIR})
