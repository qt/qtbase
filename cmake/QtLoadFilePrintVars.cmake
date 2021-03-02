# Load a file and print variables and their values
#
# IN_FILE: path to a file to be included
# VARIABLES: list of variables to be printed

cmake_minimum_required(VERSION 3.16)
include("${IN_FILE}")

# Print a magic comment that the caller must look for
message(STATUS "---QtLoadFilePrintVars---")

# Print the variables
foreach(v IN LISTS VARIABLES)
    message(STATUS "${v} ${${v}}")
endforeach()
