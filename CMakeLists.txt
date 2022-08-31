cmake_minimum_required(VERSION 3.14)

project(hciproxy)

add_executable(${PROJECT_NAME} main.c)

target_link_libraries(${PROJECT_NAME} PRIVATE bluetooth)
