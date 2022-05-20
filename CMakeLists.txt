cmake_minimum_required(VERSION 3.22)
project(extended_lab_5 C)

set(CMAKE_C_STANDARD 11)

include_directories(src)

add_executable(extended_lab_5
        src/LineParser.c
        src/LineParser.h
        src/looper.c
        src/myShell.c)
