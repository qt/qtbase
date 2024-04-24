# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(QT_FEATURE_desktopservices)
    add_subdirectory(qdesktopservices)
endif()
add_subdirectory(qdoublevalidator)
add_subdirectory(qintvalidator)
add_subdirectory(qregularexpressionvalidator)
add_subdirectory(qtexturefilereader)
if(QT_FEATURE_undogroup)
    add_subdirectory(qundogroup)
endif()
if(QT_FEATURE_undocommand)
    add_subdirectory(qundostack)
endif()
