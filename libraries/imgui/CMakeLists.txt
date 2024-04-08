cmake_minimum_required(VERSION 3.7)

project(imgui)

file(GLOB IMGUI_SRC "imgui/*.cpp")
list(APPEND IMGUI_SRC "imgui/misc/cpp/imgui_stdlib.cpp")
set(
    IMGUI_BACKENDS_SRC
    "imgui/backends/imgui_impl_vulkan.cpp"
    "imgui/backends/imgui_impl_sdl3.cpp"
)
add_library(imgui STATIC ${IMGUI_SRC} ${IMGUI_BACKENDS_SRC})
target_include_directories(imgui PUBLIC "imgui")
target_link_libraries(imgui PRIVATE SDL3::SDL3)