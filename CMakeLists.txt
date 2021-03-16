cmake_minimum_required(VERSION 3.16)
project(gucilisp)

add_compile_options(-Wall -Wextra -Werror)

add_executable(gucilisp main.cpp)
target_link_libraries(gucilisp PUBLIC readline)
target_compile_features(gucilisp PUBLIC cxx_std_20)
target_include_directories(gucilisp PUBLIC ${CMAKE_SOURCE_DIR})

add_library(catch2 third_party/catch2/catch.cpp)
target_compile_features(catch2 PUBLIC cxx_std_20)

add_subdirectory(parse)
