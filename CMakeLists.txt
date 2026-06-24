cmake_minimum_required(VERSION 3.24)
project(vulkanTest)

find_package(Vulkan REQUIRED)

set(SOURCES main.c)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} PUBLIC Vulkan::Vulkan)
