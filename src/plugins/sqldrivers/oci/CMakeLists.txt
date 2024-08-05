# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QOCIDriverPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QOCIDriverPlugin
    OUTPUT_NAME qsqloci
    PLUGIN_TYPE sqldrivers
    SOURCES
        main.cpp
        qsql_oci.cpp qsql_oci_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Oracle::OCI
        Qt::Core
        Qt::CorePrivate
        Qt::SqlPrivate
)

## Scopes:
#####################################################################

qt_internal_force_macos_intel_arch(QOCIDriverPlugin)
