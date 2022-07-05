# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

add_subdirectory(qdir)
add_subdirectory(qdiriterator)
add_subdirectory(qfile)
add_subdirectory(qfileinfo)
add_subdirectory(qiodevice)
if(QT_FEATURE_process)
    add_subdirectory(qprocess)
endif()
add_subdirectory(qtemporaryfile)
add_subdirectory(qtextstream)
add_subdirectory(qurl)
