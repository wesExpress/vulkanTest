cmake_minimum_required(VERSION 3.24)
set(CMAKE_C_STANDARD 99)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(vulkanTest)

add_subdirectory(lib/DarkMatter)

set(SOURCES main.c)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC lib/)
target_link_libraries(${PROJECT_NAME} PUBLIC DarkMatter)
