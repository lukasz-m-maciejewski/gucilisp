add_executable(ast_eval_utils_test ast_eval_utils.test.cpp)
target_link_libraries(ast_eval_utils_test PRIVATE catch2)
target_include_directories(ast_eval_utils_test PUBLIC ${CMAKE_SOURCE_DIR} "${CMAKE_SOURCE_DIR}/third_party/")
