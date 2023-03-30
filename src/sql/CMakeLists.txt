# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## Sql Module:
#####################################################################

qt_internal_add_module(Sql
    PLUGIN_TYPES sqldrivers
    SOURCES
        compat/removed_api.cpp
        kernel/qsqlcachedresult.cpp kernel/qsqlcachedresult_p.h
        kernel/qsqldatabase.cpp kernel/qsqldatabase.h
        kernel/qsqldriver.cpp kernel/qsqldriver.h kernel/qsqldriver_p.h
        kernel/qsqldriverplugin.cpp kernel/qsqldriverplugin.h
        kernel/qsqlerror.cpp kernel/qsqlerror.h
        kernel/qsqlfield.cpp kernel/qsqlfield.h
        kernel/qsqlindex.cpp kernel/qsqlindex.h
        kernel/qsqlnulldriver_p.h
        kernel/qsqlquery.cpp kernel/qsqlquery.h
        kernel/qsqlrecord.cpp kernel/qsqlrecord.h
        kernel/qsqlresult.cpp kernel/qsqlresult.h kernel/qsqlresult_p.h
        kernel/qtsqlglobal.h kernel/qtsqlglobal_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_FOREACH
        QT_NO_USING_NAMESPACE
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES
        Qt::Core
    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
    NO_PCH_SOURCES
        "compat/removed_api.cpp"
    PRECOMPILED_HEADER
        "../corelib/global/qt_pch.h"
    GENERATE_CPP_EXPORTS
)

## Scopes:
#####################################################################

qt_internal_extend_target(Sql CONDITION MSVC AND (TEST_architecture_arch STREQUAL "i386")
    LINK_OPTIONS
        "/BASE:0x62000000"
)

qt_internal_extend_target(Sql CONDITION QT_FEATURE_sqlmodel
    SOURCES
        models/qsqlquerymodel.cpp models/qsqlquerymodel.h models/qsqlquerymodel_p.h
        models/qsqlrelationaldelegate.cpp models/qsqlrelationaldelegate.h
        models/qsqlrelationaltablemodel.cpp models/qsqlrelationaltablemodel.h
        models/qsqltablemodel.cpp models/qsqltablemodel.h models/qsqltablemodel_p.h
)
qt_internal_add_docs(Sql
    doc/qtsql.qdocconf
)

# include the snippet projects for developer-builds
if(QT_FEATURE_private_tests)
    add_subdirectory(doc/snippets/code)
    add_subdirectory(doc/snippets/sqldatabase)
endif()
