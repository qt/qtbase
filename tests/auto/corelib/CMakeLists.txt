# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(kernel)
if(NOT UIKIT)
    add_subdirectory(animation)
    add_subdirectory(global)
    add_subdirectory(io)
    add_subdirectory(ipc)
    add_subdirectory(itemmodels)
    add_subdirectory(mimetypes)
    add_subdirectory(plugin)
    add_subdirectory(serialization)
    add_subdirectory(text)
    add_subdirectory(thread)
    add_subdirectory(time)
    add_subdirectory(tools)
endif()
add_subdirectory(platform)
