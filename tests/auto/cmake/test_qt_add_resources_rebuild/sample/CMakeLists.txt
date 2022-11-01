cmake_minimum_required(VERSION 3.16)
project(sample LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core)

set(source "${CMAKE_BINARY_DIR}/main.cpp")
file(GENERATE OUTPUT "${source}" CONTENT "int main() { return 0; }")

qt_add_executable(${PROJECT_NAME} ${source})

# This is a poor man's implementation of qt_add_lupdate.
set(input "${CMAKE_SOURCE_DIR}/input.ts")
set(output "${CMAKE_BINARY_DIR}/output.qm")
add_custom_command(
    OUTPUT "${output}"
    COMMAND ${CMAKE_COMMAND} -E copy "${input}" "${output}"
    DEPENDS "${input}"
    VERBATIM
)

# This is where the bug happened before. Adding the target dependency properties used the target
# as an order-only dependency, instead of depending on the actual dependency file.
set_source_files_properties("${output}"
    PROPERTIES _qt_resource_target_dependency "output_target")

add_custom_target(output_target
    DEPENDS "${output}"
)

qt_add_resources(${PROJECT_NAME} "res"
    PREFIX "/"
    BASE "${CMAKE_CURRENT_BINARY_DIR}"
    FILES "${output}"
)

# Write out the location of the binary so its timestamp can be checked by the driving parent
# project.
set(target_file_out "${CMAKE_BINARY_DIR}/targets.txt")
add_custom_target(all_built ALL
    COMMAND
        ${CMAKE_COMMAND} -E echo "$<TARGET_FILE:${PROJECT_NAME}>" > "${target_file_out}"
    VERBATIM
)
# Make sure the file path is written out after the executable is linked.
add_dependencies(all_built ${PROJECT_NAME})
