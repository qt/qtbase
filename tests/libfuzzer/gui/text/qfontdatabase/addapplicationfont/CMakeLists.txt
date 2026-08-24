# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(addapplicationfont LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui)

# This test uses the BrokenType ttf-otf-mutator as an in-process custom mutator.
# The user is expected to have checked out https://github.com/googleprojectzero/BrokenType
# and to point the BROKENTYPE_PATH environment variable at that checkout.
if(NOT DEFINED ENV{BROKENTYPE_PATH})
    message(FATAL_ERROR
        "The BROKENTYPE_PATH environment variable must point at a checkout of "
        "the BrokenType repository (https://github.com/googleprojectzero/BrokenType). "
        "See README.md for details.")
endif()

set(brokentype_dir "$ENV{BROKENTYPE_PATH}/ttf-otf-mutator")
if(NOT EXISTS "${brokentype_dir}/sfnt_mutator.cpp")
    message(FATAL_ERROR
        "Could not find the ttf-otf-mutator sources under \"${brokentype_dir}\". "
        "Make sure BROKENTYPE_PATH points at the root of the BrokenType checkout.")
endif()

set(brokentype_sources
    "${brokentype_dir}/mutator.cpp"
    "${brokentype_dir}/random.cpp"
    "${brokentype_dir}/sfnt_font.cpp"
    "${brokentype_dir}/sfnt_mutator.cpp"
)

# The tool relies on std::random_shuffle (removed in C++17) and on multi-character
# character constants, so build its translation units as C++14 and quiet the
# expected warnings. These flags are scoped to the BrokenType sources only.
set_source_files_properties(${brokentype_sources} PROPERTIES
    COMPILE_OPTIONS "-std=c++14;-Wno-multichar"
)

qt_add_executable(addapplicationfont
    main.cpp
    ${brokentype_sources}
)

target_include_directories(addapplicationfont PRIVATE
    "${brokentype_dir}"
)

target_link_libraries(addapplicationfont PUBLIC
    Qt::Core
    Qt::Gui
)
if(DEFINED ENV{LIB_FUZZING_ENGINE})
    target_link_libraries(addapplicationfont PRIVATE
        $ENV{LIB_FUZZING_ENGINE}
    )
else()
    target_link_libraries(addapplicationfont PRIVATE
        -fsanitize=fuzzer
    )
endif()
