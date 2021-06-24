add_executable(gucilisp main.cpp)
target_link_libraries(gucilisp PUBLIC readline fmt::fmt)
target_compile_features(gucilisp PUBLIC cxx_std_20)
target_include_directories(gucilisp PUBLIC ${CMAKE_SOURCE_DIR})


add_subdirectory(parse)
add_subdirectory(eval)

add_executable(curses_demo curses_demo.cpp)
target_link_libraries(curses_demo PUBLIC ncurses)
target_compile_features(curses_demo PUBLIC cxx_std_20)
target_include_directories(curses_demo PUBLIC ${CMAKE_SOURCE_DIR})
