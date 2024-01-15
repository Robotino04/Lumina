cmake_minimum_required(VERSION 3.7)


add_executable(TrialGround "src/main.cpp")
target_link_libraries(TrialGround PUBLIC LuminaEssence)

#[[
#----------------| Shader Compilation |----------------#

# find all shaders
file(GLOB_RECURSE TRIAL_GROUND_SHADERS_SRC "*.vert" "*.frag")

message(STATUS "Shader files found:")
foreach(SHADER_IN ${TRIAL_GROUND_SHADERS_SRC})
    # generate the output filename
    file(RELATIVE_PATH SHADER_REL ${CMAKE_CURRENT_SOURCE_DIR} ${SHADER_IN})
    set(SHADER_OUT "${CMAKE_CURRENT_BINARY_DIR}/${SHADER_REL}.spv")

    get_filename_component(SHADER_DIR ${SHADER_OUT} DIRECTORY)

    # make sure the directory exists
    file(MAKE_DIRECTORY ${SHADER_DIR})

    message(STATUS "\t${SHADER_REL}")
    # add a build command for each shader file
    add_custom_command(
        TARGET Sandbox
        COMMAND "${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}" "-V" "-o" "${SHADER_OUT}" "${SHADER_IN}"
        COMMENT "Compiling shader file \"${SHADER_REL}\" to \"${SHADER_REL}.spv\"..."
        MAIN_DEPENDENCY ${SHADER_IN}
        VERBATIM
    )
endforeach(SHADER_IN)

message(STATUS "Compiling shaders using \"${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}\"")

#]]