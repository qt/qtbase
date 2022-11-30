# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#
# Hand crafted file based on selfcover.pri
#

# Overwrite CMake compiler
function(__qt_testlib_patch_compiler_name variable)
    get_filename_component(name ${${variable}} NAME)
    set(${variable} cs${name} PARENT_SCOPE)
endfunction()

if (FEATURE_testlib_selfcover)
    __qt_testlib_patch_compiler_name(CMAKE_C_COMPILER)
    __qt_testlib_patch_compiler_name(CMAKE_CXX_COMPILER)
    __qt_testlib_patch_compiler_name(CMAKE_CXX_COMPILER_AR)
    __qt_testlib_patch_compiler_name(CMAKE_CXX_COMPILER_RANLIB)
endif()

# This enables verification that testlib itself is adequately tested,
# as a grounds for trusting that testing with it is useful.
function(qt_internal_apply_testlib_coverage_options target)
    if (NOT FEATURE_testlib_selfcover)
        return()
    endif()
    # Exclude all non-testlib source from coverage instrumentation:
    set(testlib_coverage_options
        --cs-exclude-file-abs-wildcard="${${CMAKE_PROJECT_NAME}_SOURCE_DIR}/*"
        --cs-include-file-abs-wildcard="*/src/testlib/*"
        --cs-mcc # enable Multiple Condition Coverage
        --cs-mcdc # enable Multiple Condition / Decision Coverage
    # (recommended for ISO 26262 ASIL A, B and C -- highly recommended for ASIL D)
    # https://doc.qt.io/coco/code-coverage-analysis.html#mc-dc
    )
    target_compile_options(${target} PRIVATE
        ${testlib_coverage_options}
    )

    target_link_options(${target} PRIVATE
        ${testlib_coverage_options}
    )
endfunction()
