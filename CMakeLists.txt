cmake_minimum_required(VERSION 3.12)
project(Esercizi_SO C)

set(CMAKE_C_STANDARD 90)

add_executable(Test test.c settings_reader.c settings_reader.h macros.h)