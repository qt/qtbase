# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(qpainterpath)
add_subdirectory(qpainterpathstroker)
add_subdirectory(qcolor)
add_subdirectory(qbrush)
add_subdirectory(qregion)
add_subdirectory(qpagelayout)
add_subdirectory(qpageranges)
add_subdirectory(qpagesize)
add_subdirectory(qpainter)
add_subdirectory(qpdfwriter)
add_subdirectory(qpen)
add_subdirectory(qpaintengine)
add_subdirectory(qtransform)
add_subdirectory(qpolygon)

# QTBUG-87669
if(NOT ANDROID)
    add_subdirectory(qcolorspace)
    add_subdirectory(qcolortransform)
endif()
if(QT_FEATURE_private_tests)
    add_subdirectory(qpathclipper)
endif()
