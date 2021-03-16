add_executable(parse_test parse.test.cpp)
target_link_libraries(parse_test PRIVATE catch2)
target_include_directories(parse_test PUBLIC ${CMAKE_SOURCE_DIR} "${CMAKE_SOURCE_DIR}/third_party/")
