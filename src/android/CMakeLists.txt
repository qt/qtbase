# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Only build jars during Android build and install the templates for the
# androiddeployqt tool.
# android
if (ANDROID)
    add_subdirectory(jar)
    add_subdirectory(java)
    add_subdirectory(templates)
endif()

