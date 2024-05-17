# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS BuildInternals Core)

set(CMAKE_AUTOMOC FALSE)

qt_manual_moc(moc_files testclass.h
    DEFINITIONS
        -DMY_FIRST_DEF
        MY_SECOND_DEF
        -DMY_THIRD_DEF=1
        MY_FOURTH_DEF=1
)

add_custom_target(verify_testclass ALL COMMAND ${CMAKE_COMMAND}
    "-DMOC_ARGS=moc_testclass.cpp_parameters$<$<BOOL:$<CONFIG>>:_$<CONFIG>>"
    "-DDEFINITIONS=MY_FIRST_DEF;MY_SECOND_DEF;MY_THIRD_DEF=1;MY_FOURTH_DEF=1"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/VerifyDefines.cmake"
    VERBATIM
)

add_library(MyInterfaceLib INTERFACE)
target_compile_definitions(MyInterfaceLib INTERFACE -DMY_TRANSITIVE_DEF=1)

add_library(MyLib SHARED testclass1.h testclass.cpp)
target_link_libraries(MyLib PRIVATE Qt6::Core MyInterfaceLib)
target_compile_definitions(MyLib PRIVATE
    -DMY_FIRST_DEF
    MY_SECOND_DEF
    -DMY_THIRD_DEF=1
    MY_FOURTH_DEF=1
)

add_library(MyLib2 SHARED testclass1.h testclass.cpp)
target_link_libraries(MyLib2 PRIVATE Qt6::Core)
target_compile_definitions(MyLib2 PRIVATE
    -DMY_FOREIGN_DEF
)

qt_manual_moc(moc_files testclass1.h TARGETS MyLib MyLib2)
target_sources(MyLib PRIVATE ${moc_files})
target_sources(MyLib2 PRIVATE ${moc_files})

string(JOIN ";" expected
    "MY_FIRST_DEF"
    "MY_SECOND_DEF"
    "MY_THIRD_DEF=1"
    "MY_FOURTH_DEF=1"
    "MY_TRANSITIVE_DEF=1"
    "MY_FOREIGN_DEF"
)

add_custom_target(verify_testclass1 ALL COMMAND ${CMAKE_COMMAND}
    "-DMOC_ARGS=moc_testclass1.cpp_parameters$<$<BOOL:$<CONFIG>>:_$<CONFIG>>"
    "-DDEFINITIONS=${expected}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/VerifyDefines.cmake"
    VERBATIM
)
