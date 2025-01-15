# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(io)
add_subdirectory(itemmodels)
add_subdirectory(json)
if(QT_FEATURE_mimetype)
    add_subdirectory(mimetypes)
endif()
add_subdirectory(kernel)
add_subdirectory(text)
add_subdirectory(thread)
add_subdirectory(time)
add_subdirectory(tools)
add_subdirectory(plugin)
add_subdirectory(serialization)
add_subdirectory(platform)
