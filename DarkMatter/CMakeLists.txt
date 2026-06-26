cmake_minimum_required(VERSION 3.24)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(DarkMatter)

set(SOURCES dm.c dm_glfw_window.c dm_vulkan_renderer.c vk_mem_alloc.cpp)

find_package(Vulkan REQUIRED COMPONENTS volk)

add_subdirectory(lib/glfw)

add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC lib/ lib/glfw/include)
target_link_libraries(${PROJECT_NAME} PUBLIC glfw Vulkan::Vulkan Vulkan::volk)
