# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

if(NOT DEFINITIONS)
    message(FATAL_ERROR "No definitions are provided to test")
endif()

if(NOT MOC_ARGS)
    message(FATAL_ERROR "The moc RSP file is not specified")
endif()

file(READ "${MOC_ARGS}" moc_args_data)

string(REPLACE "\n" ";" moc_args_data "${moc_args_data}")

foreach(def IN LISTS DEFINITIONS)
    set(found FALSE)
    foreach(data IN LISTS moc_args_data)
        if(data MATCHES "^(-D)?${def}")
            set(found TRUE)
            break()
        endif()
    endforeach()
    if(NOT found)
        message(FATAL_ERROR "The ${def} is missing in the moc argument list:\n${moc_args_data}")
    endif()
endforeach()

