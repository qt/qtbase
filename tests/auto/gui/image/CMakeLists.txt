# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from image.pro.

# QTBUG-87669 # special case
if(TARGET Qt::Network AND NOT ANDROID)
    add_subdirectory(qimagereader)
endif()
if(QT_FEATURE_ico)
    add_subdirectory(qicoimageformat)
endif()
add_subdirectory(qpixmap)
add_subdirectory(qimage)
add_subdirectory(qimageiohandler)
add_subdirectory(qimagewriter)
add_subdirectory(qmovie)
add_subdirectory(qpicture)
add_subdirectory(qiconhighdpi)
if(QT_FEATURE_private_tests)
    add_subdirectory(qpixmapcache)
endif()
# QTBUG-87669 # special case
if(NOT ANDROID)
    add_subdirectory(qicon)
endif()
