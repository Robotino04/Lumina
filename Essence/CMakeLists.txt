cmake_minimum_required(VERSION 3.7)

find_package(Vulkan REQUIRED)

file(GLOB_RECURSE LUMINA_ESSENCE_SRC "src/*.cpp" "src/*.c")
file(GLOB_RECURSE LUMINA_ESSENCE_HEADERS "include/*.hpp" "include/*.h")

add_library(LuminaEssence STATIC ${LUMINA_ESSENCE_SRC} ${LUMINA_ESSENCE_HEADERS})
target_include_directories(LuminaEssence PUBLIC "include")

target_compile_features(LuminaEssence PUBLIC cxx_std_20)

target_link_libraries(LuminaEssence PUBLIC glm glfw Vulkan::Vulkan)