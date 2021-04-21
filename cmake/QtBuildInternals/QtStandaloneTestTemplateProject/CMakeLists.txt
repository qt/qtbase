cmake_minimum_required(VERSION 3.16)
project(qt_single_test VERSION 6.0.0 LANGUAGES C CXX ASM)

find_package(Qt6 REQUIRED COMPONENTS BuildInternals)

include(${CMAKE_CURRENT_LIST_DIR}/Main.cmake)

# Get the absolute path of the passed-in project dir, relative to the current working directory
# of the calling script, rather than relative to this source directory.
# The calling script sets PWD. If not set, just use the passed-in value as-is.
if(DEFINED PWD)
    get_filename_component(absolute_project_path "${QT_STANDALONE_TEST_PATH}" ABSOLUTE
                           BASE_DIR "${PWD}")
else()
    set(absolute_project_path "${QT_STANDALONE_TEST_PATH}")
endif()

# Add the test project path as a subdirectory project.
add_subdirectory("${absolute_project_path}" "build_dir")
