# This script writes its arguments to the file determined by OUT_FILE.
# Each argument appears on a separate line.
# This is used for writing the config.opt file.
#
# This script takes the following arguments:
# IN_FILE: The input file. The whole command line as one string.
# OUT_FILE: The output file. One argument per line.
# SKIP_ARGS: Number of arguments to skip from the front of the arguments list.
# IGNORE_ARGS: List of arguments to be ignored, i.e. that are not written.

cmake_minimum_required(VERSION 3.16)

# Read arguments from IN_FILE and separate them.
file(READ "${IN_FILE}" raw_args)
separate_arguments(args NATIVE_COMMAND "${raw_args}")

# Skip arguments if requested
if(DEFINED SKIP_ARGS)
    foreach(i RANGE 1 ${SKIP_ARGS})
        list(POP_FRONT args)
    endforeach()
endif()

# Write config.opt
set(content "")
foreach(arg IN LISTS args)
    if(NOT arg IN_LIST IGNORE_ARGS)
        string(APPEND content "${arg}\n")
    endif()
endforeach()

file(WRITE "${OUT_FILE}" "${content}")
