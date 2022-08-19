# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_plugin(QIBaseDriverPlugin
    OUTPUT_NAME qsqlibase
    PLUGIN_TYPE sqldrivers
    SOURCES
        main.cpp
        qsql_ibase.cpp qsql_ibase_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Interbase::Interbase
        Qt::Core
        Qt::CorePrivate
        Qt::SqlPrivate)

qt_internal_force_macos_intel_arch(QIBaseDriverPlugin)
