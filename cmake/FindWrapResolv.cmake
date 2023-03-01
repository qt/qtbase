# Copyright (C) 2022 The Qt Company Ltd.
# Copyright (C) 2023 Intel Corpotation.
# SPDX-License-Identifier: BSD-3-Clause

# We can't create the same interface imported target multiple times, CMake will complain if we do
# that. This can happen if the find_package call is done in multiple different subdirectories.
if(TARGET WrapResolv::WrapResolv)
    set(WrapResolv_FOUND ON)
    return()
endif()

set(WrapResolv_FOUND OFF)

include(CheckCXXSourceCompiles)
include(CMakePushCheckState)

if(QNX)
    find_library(LIBRESOLV socket)
else()
    find_library(LIBRESOLV resolv)
endif()

cmake_push_check_state()
if(LIBRESOLV)
    list(APPEND CMAKE_REQUIRED_LIBRARIES "${LIBRESOLV}")
endif()

check_cxx_source_compiles("
#include <netinet/in.h>
#include <resolv.h>

int main(int, char **)
{
    res_init();
}
" HAVE_RES_INIT)

cmake_pop_check_state()

if(HAVE_RES_INIT)
    set(WrapResolv_FOUND ON)
    add_library(WrapResolv::WrapResolv INTERFACE IMPORTED)
    if(LIBRESOLV)
        target_link_libraries(WrapResolv::WrapResolv INTERFACE "${LIBRESOLV}")
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WrapResolv DEFAULT_MSG WrapResolv_FOUND)
