# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Check that a framework module's <Module>Depends and qt<module>exports.h headers
# resolve through the generated forwarders when included with a bare include.
#
# A forwarder has to resolve no matter how the consuming build system spells the
# include flags. The framework can arrive as -F (Clang's angled group) or as
# -iframework (system group), and the forwarder dirs as -I or -isystem, in any
# mix; the framework flag can sit anywhere among the forwarder dirs; and the naked
# include dir may or may not be on the line (it is when the consumer also pulls in
# a header-only module). The tests below drive the compiler directly with each of
# those flag shapes.

find_package(Qt6 COMPONENTS ALL_QT_MODULES)
set(framework_dir "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_LIBS}")

# The naked include dir: <prefix>/include, parent of every per-module include dir.
set(naked_include_dir "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_HEADERS}")

# The two axes we permute over.
set(include_flags "-I" "-isystem")
set(framework_flags "-F" "-iframework")

# Compile a source with a given set of include flags, syntax-only.
function(add_forwarding_header_test name source)
    add_test(NAME test_framework_forwarding_headers_${name}
        COMMAND "${CMAKE_CXX_COMPILER}"
            -x c++
            -std=c++17
            -fsyntax-only
            ${ARGN}
            "${source}"
    )
endfunction()

# Set ${out} in the caller to the bare-include lines for a module, or empty if the
# module ships no forwardable headers in its framework Headers dir.
function(module_bare_includes out include_name lower headers_dir)
    set(content "")
    if(EXISTS "${headers_dir}/${include_name}Depends")
        string(APPEND content "#include <${include_name}Depends>\n")
    endif()
    if(EXISTS "${headers_dir}/qt${lower}exports.h")
        string(APPEND content "#include <qt${lower}exports.h>\n")
    endif()
    set(${out} "${content}" PARENT_SCOPE)
endfunction()

# For every include-flag family (-I/-isystem) crossed with every framework-flag
# family (-F/-iframework), add a test for each position the single framework flag
# can take among the given include dirs (before all, between each pair, after all).
# ARGN is the list of include dirs, in order.
function(add_flag_shape_tests name_prefix source)
    set(incdirs ${ARGN})
    list(LENGTH incdirs dir_count)
    foreach(iflag IN LISTS include_flags)
        foreach(fflag IN LISTS framework_flags)
            string(REGEX REPLACE "^-+" "" ilabel "${iflag}")
            string(REGEX REPLACE "^-+" "" flabel "${fflag}")
            foreach(fw_pos RANGE ${dir_count})
                set(flags "")
                set(pos 0)
                foreach(dir IN LISTS incdirs)
                    if(pos EQUAL fw_pos)
                        list(APPEND flags "${fflag}" "${framework_dir}")
                    endif()
                    list(APPEND flags "${iflag}" "${dir}")
                    math(EXPR pos "${pos} + 1")
                endforeach()
                if(fw_pos EQUAL dir_count)
                    list(APPEND flags "${fflag}" "${framework_dir}")
                endif()
                add_forwarding_header_test(
                    "${name_prefix}_${ilabel}_${flabel}_fwpos${fw_pos}"
                    "${source}" ${flags})
            endforeach()
        endforeach()
    endforeach()
endfunction()

# Collected in module order, for the single- and multi-module matrices below.
# collected_includes holds each module's bare-include lines (no semicolons, so a
# list preserves them); the combined multi-module source is built from those.
set(collected_lower "")
set(collected_include_dir "")
set(collected_source "")
set(collected_includes "")

foreach(module_target IN LISTS QT_ALL_MODULES_VERSIONED_FOUND_VIA_FIND_PACKAGE)
    qt_internal_module_info(module "${module_target}")
    qt_internal_get_framework_info(fw "${module_target}")

    set(module_headers_dir "${framework_dir}/${fw_header_dir}")
    set(module_include_dir "${naked_include_dir}/${module_include_name}")

    module_bare_includes(includes
        "${module_include_name}" "${module_lower}" "${module_headers_dir}")

    # Skip frameworks that don't have the headers in their Headers dir.
    if(NOT includes)
        continue()
    endif()

    set(source_file "${CMAKE_CURRENT_BINARY_DIR}/bare_include_${module_lower}.cpp")
    file(GENERATE OUTPUT "${source_file}" CONTENT "${includes}int main() { return 0; }\n")

    # Baseline, every module: the shape CMake's Ninja and Makefile generators
    # emit, which resolves today. The flag-shape matrix below runs on one module.
    # The naked include dir goes in via -idirafter (Clang's last-searched group,
    # as Qt6::Platform passes it on framework builds), so a module that pulls in a
    # header-only module still finds that module's umbrella header without the naked
    # dir disturbing the forwarder-vs-framework order.
    add_forwarding_header_test(${module_lower} "${source_file}"
        -iframework "${framework_dir}"
        -isystem "${module_include_dir}"
        -idirafter "${naked_include_dir}"
    )

    list(APPEND collected_lower "${module_lower}")
    list(APPEND collected_include_dir "${module_include_dir}")
    list(APPEND collected_source "${source_file}")
    list(APPEND collected_includes "${includes}")
endforeach()

# Single-module matrix, on the first module that has forwarders. The behaviour is
# a property of the flag shape, not of the module, so one module exercises every
# combination, with and without the naked include dir.
list(LENGTH collected_lower module_count)
if(module_count GREATER_EQUAL 1)
    list(GET collected_lower 0 m)
    list(GET collected_include_dir 0 m_inc)
    list(GET collected_source 0 m_src)

    add_flag_shape_tests(${m} "${m_src}" "${m_inc}")
    add_flag_shape_tests(${m}_naked "${m_src}" "${m_inc}" "${naked_include_dir}")
endif()

# Multi-module matrix, on the first two modules that have forwarders. Two modules
# share one framework dir (a single framework flag after the build system dedups
# it) but keep their own forwarder include dirs, so the interesting variable is
# where that one framework flag sits among the two forwarder dirs. A bare include
# from each module has to resolve in every position, with and without the naked dir.
if(module_count GREATER_EQUAL 2)
    list(GET collected_lower 0 a)
    list(GET collected_lower 1 b)
    list(GET collected_include_dir 0 a_inc)
    list(GET collected_include_dir 1 b_inc)
    list(GET collected_includes 0 a_includes)
    list(GET collected_includes 1 b_includes)

    # Combined source with the bare includes of both modules.
    set(multi_src "${CMAKE_CURRENT_BINARY_DIR}/bare_include_${a}_${b}.cpp")
    file(GENERATE OUTPUT "${multi_src}"
        CONTENT "${a_includes}${b_includes}int main() { return 0; }\n")

    add_flag_shape_tests(multi_${a}_${b} "${multi_src}" "${a_inc}" "${b_inc}")
    add_flag_shape_tests(multi_${a}_${b}_naked "${multi_src}"
        "${a_inc}" "${b_inc}" "${naked_include_dir}")
endif()
