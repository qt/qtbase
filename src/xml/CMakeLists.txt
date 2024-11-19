# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## Xml Module:
#####################################################################

qt_internal_add_module(Xml
    SOURCES
        qtxmlglobal.h
    DEFINES
        QT_NO_CONTEXTLESS_CONNECT
        QT_NO_FOREACH
        QT_NO_USING_NAMESPACE
        QT_USE_NODISCARD_FILE_OPEN
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES
        Qt::Core
    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
)

qt_internal_extend_target(Xml CONDITION QT_FEATURE_dom
    SOURCES
        compat/removed_api.cpp
        dom/qdom.cpp dom/qdom.h dom/qdom_p.h
        dom/qdomhelpers.cpp dom/qdomhelpers_p.h
    NO_PCH_SOURCES
        compat/removed_api.cpp
)

qt_internal_extend_target(Xml CONDITION MSVC AND (TEST_architecture_arch STREQUAL "i386")
    LINK_OPTIONS
        "/BASE:0x61000000"
)

qt_internal_add_docs(Xml
    doc/qtxml.qdocconf
)

