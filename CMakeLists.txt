cmake_minimum_required(VERSION 3.24)
set(CMAKE_C_STANDARD 99)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(vulkanTest)

find_package(Vulkan REQUIRED COMPONENTS volk)

add_subdirectory(lib/glfw)

set(SOURCES main.c)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC lib/ lib/glfw/include)
target_link_libraries(${PROJECT_NAME} PUBLIC glfw Vulkan::Vulkan Vulkan::volk)
