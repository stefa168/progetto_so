cmake_minimum_required(VERSION 3.12)
project(Esercizi_SO C)

set(CMAKE_C_STANDARD 90)

set(general_imports utilities/macros.h utilities/utils.c utilities/utils.h utilities/ipc_utils.c utilities/ipc_utils.h utilities/types.h)
set(settings_reader utilities/settings_reader.c utilities/settings_reader.h)

add_executable(Test test.c)

add_executable(Gestore gestore.c gestore.h ${settings_reader} ${general_imports})
add_executable(Studente Studente.c Studente.h ${general_imports})
add_dependencies(Gestore Studente)