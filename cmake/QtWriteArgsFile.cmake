# This script writes its arguments to the file determined by OUT_FILE.
# Each argument appears on a separate line.
# This is used for writing the config.opt file.
#
# This script takes the following arguments:
# OUT_FILE: The output file.
# SKIP_ARGS: Number of arguments to skip from the front of the arguments list.
# IGNORE_ARGS: List of arguments to be ignored, i.e. that are not written.

cmake_minimum_required(VERSION 3.3)

# Look for the -P argument to determine the start of the actual script arguments
math(EXPR stop "${CMAKE_ARGC} - 1")
set(start 0)
foreach(i RANGE 1 ${stop})
    if(CMAKE_ARGV${i} STREQUAL "-P")
        math(EXPR start "${i} + 2")
        break()
    endif()
endforeach()

# Skip arguments if requested
if(DEFINED SKIP_ARGS)
    math(EXPR start "${start} + ${SKIP_ARGS}")
endif()

# Write config.opt
set(content "")
if(start LESS_EQUAL stop)
    foreach(i RANGE ${start} ${stop})
        set(arg ${CMAKE_ARGV${i}})
        if(NOT arg IN_LIST IGNORE_ARGS)
            string(APPEND content "${arg}\n")
        endif()
    endforeach()
endif()
file(WRITE "${OUT_FILE}" "${content}")
