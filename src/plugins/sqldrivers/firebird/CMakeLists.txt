# Copyright (C) 2026 The Qt Company Ltd.
# Copyright (C) 2026 Andreas Bacher
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_plugin(QFirebirdDriverPlugin
    OUTPUT_NAME qsqlfirebird
    PLUGIN_TYPE sqldrivers
    EXCEPTIONS
    SOURCES
        main.cpp
        qsql_firebird.cpp qsql_firebird_p.h
        qsql_firebird_array.cpp qsql_firebird_array_p.h
        qsql_firebird_driver_p.h
        qsql_firebird_helpers.cpp qsql_firebird_helpers_p.h
        qsql_firebird_result.cpp qsql_firebird_result_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
        QT_NO_CONTEXTLESS_CONNECT
    LIBRARIES
        Firebird::Client
        Qt::Core
        Qt::CorePrivate
        Qt::SqlPrivate)

qt_internal_force_macos_intel_arch(QFirebirdDriverPlugin)
