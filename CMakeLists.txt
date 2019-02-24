cmake_minimum_required(VERSION 3.10)
project(pthreads_popr C)

set(CMAKE_C_STANDARD 11)


set (CMAKE_C_FLAGS "-Wall -O3")




add_executable(serwer serwer.c worker.c serverStorage.c report.c macros.h)
target_link_libraries(serwer pthread)


add_executable(klient klient.c report.c macros.h)
target_link_libraries(klient pthread)
