cmake_minimum_required(VERSION 3.12)
project(Esercizi_SO C)

set(CMAKE_C_STANDARD 90)

add_executable(Test test.c)
add_executable(filesum file-sum.c)

#prima impostiamo tutte le dipendenze di manager
#set(manager_SOURCES test.c)
#poi tutte quelle di student
# TODO

#poi creiamo l'eseguibile con tutte le dipendenze
#add_executable(Test ${manager_SOURCES})