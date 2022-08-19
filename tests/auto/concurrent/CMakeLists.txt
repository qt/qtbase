# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from concurrent.pro.

add_subdirectory(qtconcurrentfilter)
add_subdirectory(qtconcurrentiteratekernel)
add_subdirectory(qtconcurrentfiltermapgenerated)
add_subdirectory(qtconcurrentmap)
add_subdirectory(qtconcurrentmedian)
if(NOT INTEGRITY)
    add_subdirectory(qtconcurrentrun)
    add_subdirectory(qtconcurrenttask)
endif()
add_subdirectory(qtconcurrentthreadengine)
