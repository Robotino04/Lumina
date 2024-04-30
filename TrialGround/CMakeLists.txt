cmake_minimum_required(VERSION 3.7)

set(TRIAL_GROUND_RESOURCE_TARGET "${CMAKE_CURRENT_BINARY_DIR}/resources")
set(TRIAL_GROUND_RESOURCE_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/resources")

add_custom_command(
    OUTPUT ${TRIAL_GROUND_RESOURCE_TARGET}
    COMMAND ${CMAKE_COMMAND} -E create_symlink "${TRIAL_GROUND_RESOURCE_SOURCE}" "${TRIAL_GROUND_RESOURCE_TARGET}"
    COMMENT "Linking ${TRIAL_GROUND_RESOURCE_TARGET} to ${TRIAL_GROUND_RESOURCE_SOURCE}..."
)

# ----------------| Shader Compilation |---------------- #

# find all shaders
file(GLOB_RECURSE TRIAL_GROUND_SHADERS_SRC "${TRIAL_GROUND_RESOURCE_SOURCE}/shaders/*.vert" "${TRIAL_GROUND_RESOURCE_SOURCE}/shaders/*.frag" "${TRIAL_GROUND_RESOURCE_SOURCE}/shaders/*.comp")

set(TRIAL_GROUND_SHADER_BINARIES)

message(STATUS "Shader files found:")

foreach(SHADER_IN ${TRIAL_GROUND_SHADERS_SRC})
    # generate the output filename
    file(RELATIVE_PATH SHADER_REL ${TRIAL_GROUND_RESOURCE_SOURCE} ${SHADER_IN})
    set(SHADER_OUT "${TRIAL_GROUND_RESOURCE_TARGET}/${SHADER_REL}.spv")
    list(APPEND TRIAL_GROUND_SHADER_BINARIES ${SHADER_OUT})

    message(STATUS "\t${SHADER_REL}")

    # add a build command for each shader file
    add_custom_command(
        OUTPUT ${SHADER_OUT}
        COMMAND "${Vulkan_GLSLC_EXECUTABLE}" "-o" "${SHADER_OUT}" "${SHADER_IN}"
        COMMENT "Compiling shader file \"${SHADER_REL}\" to \"${SHADER_REL}.spv\"..."
        MAIN_DEPENDENCY ${SHADER_IN}
        DEPENDS ${TRIAL_GROUND_RESOURCE_TARGET}
        VERBATIM
    )
endforeach(SHADER_IN)

# ----------------| Trial Ground |---------------- #
add_executable(TrialGround "src/main.cpp" ${TRIAL_GROUND_SHADER_BINARIES})
target_link_libraries(TrialGround PUBLIC LuminaEssence)