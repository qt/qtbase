# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(json)
add_subdirectory(qcborstreamreader)
add_subdirectory(qcborstreamwriter)
add_subdirectory(qcborvalue)
add_subdirectory(qcborvalue_json)
if(TARGET Qt::Gui)
    add_subdirectory(qdatastream)
    add_subdirectory(qdatastream_core_pixmap)
endif()
if(TARGET Qt::Network)
    add_subdirectory(qtextstream)
endif()
if(TARGET Qt::Gui AND TARGET Qt::Network AND TARGET Qt::Xml AND NOT INTEGRITY AND NOT QNX)
    add_subdirectory(qxmlstream)
endif()
