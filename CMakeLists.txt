cmake_minimum_required(VERSION 3.16)
project(Project3 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# main.c drives the sample. disk.c supplies the reusable persistence operations.
add_executable(Project3 main.c disk.c
        shell.c
        fs.c
        fs.h)
