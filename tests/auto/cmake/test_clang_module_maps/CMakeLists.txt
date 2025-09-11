# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

set(modules_cache_path "${CMAKE_CURRENT_BINARY_DIR}/module-cache")

add_test(NAME test_clang_header_map_setup
    COMMAND mkdir -p "${modules_cache_path}"
)
set_tests_properties(test_clang_header_map_setup
    PROPERTIES FIXTURES_SETUP header_map_test
)
add_test(NAME test_clang_header_map_cleanup
    COMMAND rm -Rf "${modules_cache_path}"
)
set_tests_properties(test_clang_header_map_cleanup
    PROPERTIES FIXTURES_CLEANUP header_map_test
)
set(module_compile_command
    clang
    -x objective-c++
    -std=c++17
    -fmodules
    -fcxx-modules
    -fmodules-cache-path="${modules_cache_path}"
    -fsyntax-only
    -ferror-limit=0
    -Xclang
    -fdiagnostics-absolute-paths
    -fdiagnostics-show-note-include-stack
    -Wnon-modular-include-in-module
    -Wnon-modular-include-in-framework-module
    -Wincomplete-umbrella
    -Wquoted-include-in-framework-header
    -Watimport-in-framework-header
    -Wframework-include-private-from-public
    -Wincomplete-framework-module-declaration
    -Wincomplete-module
    -Werror
    -F"${QT6_INSTALL_PREFIX}/${QT6_INSTALL_LIBS}"
    -I"${QT6_INSTALL_PREFIX}/${QT6_INSTALL_HEADERS}"
)
string(REPLACE ";" " " module_compile_command "${module_compile_command}")

find_package(Qt6 COMPONENTS ALL_QT_MODULES)
foreach(module_target IN LISTS QT_ALL_MODULES_VERSIONED_FOUND_VIA_FIND_PACKAGE)
    get_target_property(module_map ${module_target} _qt_module_map)
    if(NOT module_map)
        continue()
    endif()
    qt_internal_module_info(module "${module_target}")

    add_test(NAME test_clang_header_map_${module_lower}
        COMMAND ${CMAKE_COMMAND} -E env bash -c
            "${module_compile_command} <(echo '@import ${module};')"
    )
    set_tests_properties(test_clang_header_map_${module_lower}
        PROPERTIES FIXTURES_REQUIRED header_map_test)
endforeach()
