cmake_minimum_required (VERSION 3.12)
project(iv)

include(FindPkgConfig)
pkg_search_module(NCURSES REQUIRED ncurses)

add_executable(iv
	iv.cpp
)

set_property(TARGET iv PROPERTY CXX_STANDARD 20)
target_include_directories(iv SYSTEM PUBLIC ${NCURSES_INCLUDE_DIRS})
target_link_libraries(iv ${NCURSES_LIBRARIES})
