cmake_minimum_required(VERSION 3.16)
project(${RunCMake_TEST} LANGUAGES CXX)

# To allow including the gen files in other subdirectory projects.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(${SBOM_INCLUDE_FILE}.cmake)
