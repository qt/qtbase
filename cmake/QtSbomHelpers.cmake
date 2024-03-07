# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# For now these are simple internal forwarding wrappers for the public counterparts, which are
# meant to be used in qt repo CMakeLists.txt files.
function(qt_internal_add_sbom)
    _qt_internal_add_sbom(${ARGN})
endfunction()

function(qt_internal_extend_sbom)
    _qt_internal_extend_sbom(${ARGN})
endfunction()

function(qt_internal_sbom_add_license)
    _qt_internal_sbom_add_license(${ARGN})
endfunction()

function(qt_internal_extend_sbom_dependencies)
    _qt_internal_extend_sbom_dependencies(${ARGN})
endfunction()

function(qt_find_package_extend_sbom)
    _qt_find_package_extend_sbom(${ARGN})
endfunction()
