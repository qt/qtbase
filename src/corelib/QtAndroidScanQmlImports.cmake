# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Collects every import/pragma line from .qml/.js/.mjs files under SOURCE_DIRS,
# rewrites SNAPSHOT only when the set changes, and always touches STAMP.
# Used both as a configure-time seed and as a build-time custom command.
# Run with cmake -P.

if(NOT DEFINED SNAPSHOT OR NOT DEFINED STAMP)
    message(FATAL_ERROR "SNAPSHOT and STAMP variables are required.")
endif()

set(files "")
if(DEFINED SOURCE_DIRS)
    foreach(dir IN LISTS SOURCE_DIRS)
        if(NOT IS_DIRECTORY "${dir}")
            continue()
        endif()
        file(GLOB_RECURSE found LIST_DIRECTORIES false "${dir}/*.qml" "${dir}/*.js" "${dir}/*.mjs")
        list(APPEND files ${found})
    endforeach()
endif()
list(REMOVE_DUPLICATES files)

set(imports "")
foreach(file IN LISTS files)
    file(READ "${file}" content)
    # Filter out commented imports
    # TODO: This regex is a heuristic, replace it with a qmlimportscanner depfile (QTBUG-147227)
    string(REGEX MATCHALL
        "/\\*[^*]*\\*+([^*/][^*]*\\*+)*/|(^|\n)[ \t]*(import|pragma)[ \t]+[A-Za-z\"][^\n]*"
        matches "${content}")
    foreach(match IN LISTS matches)
        if(match MATCHES "^[ \t\n]*(import|pragma)")
            # Strip trailing comments
            string(REGEX REPLACE "[ \t]*//.*$" "" match "${match}")
            string(REGEX REPLACE "[ \t]*/\\*.*$" "" match "${match}")
            string(STRIP "${match}" match)
            list(APPEND imports "${match}")
        endif()
    endforeach()
endforeach()
list(REMOVE_DUPLICATES imports)
list(SORT imports)
string(REPLACE ";" "\n" current "${imports}")

set(previous "")
if(EXISTS "${SNAPSHOT}")
    file(READ "${SNAPSHOT}" previous)
endif()

if(NOT current STREQUAL previous)
    file(WRITE "${SNAPSHOT}" "${current}")
endif()
file(TOUCH "${STAMP}")
