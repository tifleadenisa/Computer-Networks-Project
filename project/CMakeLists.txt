cmake_minimum_required(VERSION 3.21)
project(proj C)

set(CMAKE_C_STANDARD 17)

add_executable(proj server.c client.c)
