# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# QTBUG-87669
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
if(QT_FEATURE_movie)
    add_subdirectory(qmovie)
endif()
if(QT_FEATURE_picture)
    add_subdirectory(qpicture)
endif()
add_subdirectory(qiconhighdpi)
if(QT_FEATURE_private_tests)
    add_subdirectory(qpixmapcache)
endif()

add_subdirectory(qicon)
