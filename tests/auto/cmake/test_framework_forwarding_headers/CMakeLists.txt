# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Check that we can include all framework module <Module>Depends and qt<module>exports.h headers
# using bare includes, thus testing that the forwarders are correctly created.

find_package(Qt6 COMPONENTS ALL_QT_MODULES)
set(framework_dir "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_LIBS}")
set(include_dir "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_HEADERS}")

foreach(module_target IN LISTS QT_ALL_MODULES_VERSIONED_FOUND_VIA_FIND_PACKAGE)
    qt_internal_module_info(module "${module_target}")
    qt_internal_get_framework_info(fw "${module_target}")

    set(module_framework_headers_dir "${framework_dir}/${fw_header_dir}")
    set(module_include_dir "${include_dir}/${module_include_name}")

    set(source_content "")

    if(EXISTS "${module_framework_headers_dir}/${module_include_name}Depends")
        string(APPEND source_content "#include <${module_include_name}Depends>\n")
    endif()

    if(EXISTS "${module_framework_headers_dir}/qt${module_lower}exports.h")
        string(APPEND source_content "#include <qt${module_lower}exports.h>\n")
    endif()

    # Skip frameworks that don't have the headers in their Headers dir.
    if(NOT source_content)
        continue()
    endif()
    string(APPEND source_content "int main() { return 0; }\n")

    set(source_file "${CMAKE_CURRENT_BINARY_DIR}/bare_include_${module_lower}.cpp")
    file(GENERATE OUTPUT "${source_file}" CONTENT "${source_content}")

    add_test(NAME test_framework_forwarding_headers_${module_lower}
        COMMAND "${CMAKE_CXX_COMPILER}"
            -x c++
            -std=c++17
            -fsyntax-only
            -iframework "${framework_dir}"
            -isystem "${module_include_dir}"
            "${source_file}"
    )
endforeach()
