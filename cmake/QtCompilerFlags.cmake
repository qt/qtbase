# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Enable compiler warnings by default. All compilers except MSVC support -Wall -Wextra
#
# You can disable the warnings for specific targets (for instance containing 3rd party code)
# by calling qt_disable_warnings(target). This will set the QT_COMPILE_OPTIONS_DISABLE_WARNINGS
# property checked below, and is equivalent to qmake's CONFIG += warn_off.

set(_qt_compiler_warning_flags_on "")
set(_qt_compiler_warning_flags_off -w)

if (MSVC)
    list(APPEND _qt_compiler_warning_flags_on /W3)
else()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GHS") # There is no -Wextra flag for GHS compiler.
        list(APPEND _qt_compiler_warning_flags_on -Wall)
    else()
        list(APPEND _qt_compiler_warning_flags_on -Wall -Wextra)
    endif()
endif()

set(_qt_compiler_warning_flags_condition
    "$<BOOL:$<TARGET_PROPERTY:QT_COMPILE_OPTIONS_DISABLE_WARNINGS>>")
set(_qt_compiler_warning_flags_genex
    "$<IF:${_qt_compiler_warning_flags_condition},${_qt_compiler_warning_flags_off},${_qt_compiler_warning_flags_on}>")

set(_qt_compiler_warning_flags_language_condition
    "$<COMPILE_LANGUAGE:CXX,C,OBJC,OBJCXX>")
set(_qt_compiler_warning_flags_language_conditional_genex
    "$<${_qt_compiler_warning_flags_language_condition}:${_qt_compiler_warning_flags_genex}>")


# Need to replace semicolons so that the list is not wrongly expanded in the add_compile_options
# call.
string(REPLACE ";" "$<SEMICOLON>"
       _qt_compiler_warning_flags_language_conditional_genex
       "${_qt_compiler_warning_flags_language_conditional_genex}")
add_compile_options(${_qt_compiler_warning_flags_language_conditional_genex})
