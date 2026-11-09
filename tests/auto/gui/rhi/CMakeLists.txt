# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(qshader)
add_subdirectory(qrhi)
if(WIN32)
    add_subdirectory(qdxgivsyncservice)
endif()
