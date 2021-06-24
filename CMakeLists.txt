cmake_minimum_required(VERSION 3.16)

project(gucilisp)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Werror" CACHE INTERNAL "" FORCE)
set(CMAKE_CXX_FLAGS_DEBUG "-g" CACHE INTERNAL "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "-O2" CACHE INTERNAL "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS "" CACHE INTERNAL "" FORCE)

add_subdirectory(third_party/fmt EXCLUDE_FROM_ALL)

add_library(catch2 third_party/catch2/catch.cpp)
target_compile_features(catch2 PUBLIC cxx_std_20)

add_subdirectory(guci)
