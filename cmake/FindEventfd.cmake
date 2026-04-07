# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Find the Eventfd library (QNX)

# Will make the target Eventfd::Eventfd available when found.
if(TARGET Eventfd::Eventfd)
    set(Eventfd_FOUND TRUE)
    return()
endif()

find_library(Eventfd_LIBRARY NAMES "eventfd")
find_path(Eventfd_INCLUDE_DIR NAMES "sys/eventfd.h" DOC "The Eventfd Include path")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Eventfd DEFAULT_MSG Eventfd_INCLUDE_DIR Eventfd_LIBRARY)

mark_as_advanced(Eventfd_INCLUDE_DIR Eventfd_LIBRARY)

if(Eventfd_FOUND)
    add_library(Eventfd::Eventfd INTERFACE IMPORTED)
    target_link_libraries(Eventfd::Eventfd INTERFACE "${Eventfd_LIBRARY}")
    target_include_directories(Eventfd::Eventfd INTERFACE "${Eventfd_INCLUDE_DIR}")
endif()
