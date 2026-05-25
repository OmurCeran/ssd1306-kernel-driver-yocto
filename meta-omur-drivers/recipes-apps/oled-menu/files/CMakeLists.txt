cmake_minimum_required(VERSION 3.15)
project(oled-menu VERSION 1.0 LANGUAGES C)

add_executable(oled-menu oled_menu.c)
install(TARGETS oled-menu DESTINATION ${CMAKE_INSTALL_BINDIR})