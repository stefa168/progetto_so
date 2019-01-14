cmake_minimum_required(VERSION 3.12)
project(Esercizi_SO C)

set(CMAKE_C_STANDARD 90)

set(general_imports macros.h utils.c utils.h)

add_executable(Test test.c)

add_executable(Gestore gestore.c gestore.h settings_reader.c settings_reader.h ${general_imports})
add_executable(Studente Studente.c Studente.h ${general_imports})