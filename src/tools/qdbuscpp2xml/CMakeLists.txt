# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qdbuscpp2xml Tool:
#####################################################################

qt_get_tool_target_name(target_name qdbuscpp2xml)
qt_internal_add_tool(${target_name}
    TRY_RUN
    TRY_RUN_FLAGS "-V"
    TARGET_DESCRIPTION "Qt D-Bus C++ to XML Compiler"
    TOOLS_TARGET DBus
    SOURCES
        ../moc/cbordevice.h
        ../moc/collectjson.cpp ../moc/collectjson.h
        ../moc/generator.cpp ../moc/generator.h
        ../moc/moc.cpp ../moc/moc.h
        ../moc/outputrevision.h
        ../moc/parser.cpp ../moc/parser.h
        ../moc/preprocessor.cpp ../moc/preprocessor.h
        ../moc/symbols.h
        ../moc/token.cpp ../moc/token.h
        ../moc/utils.h
        qdbuscpp2xml.cpp
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_FOREACH
    INCLUDE_DIRECTORIES
        ../moc
        ../moc/../../3rdparty/tinycbor/src
    LIBRARIES
        Qt::CorePrivate
        Qt::DBusPrivate
)
qt_internal_return_unless_building_tools()
