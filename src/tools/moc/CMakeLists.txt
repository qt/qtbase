# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## moc Tool:
#####################################################################

qt_get_tool_target_name(target_name moc)
qt_internal_add_tool(${target_name}
    TRY_RUN
    CORE_LIBRARY Bootstrap
    TARGET_DESCRIPTION "Qt Meta Object Compiler"
    INSTALL_DIR "${INSTALL_LIBEXECDIR}"
    TOOLS_TARGET Core
    SOURCES
        cbordevice.h
        collectjson.cpp collectjson.h
        generator.cpp generator.h
        main.cpp
        moc.cpp moc.h
        outputrevision.h
        parser.cpp parser.h
        preprocessor.cpp preprocessor.h
        symbols.h
        token.cpp token.h
        utils.h
    DEFINES
        QT_MOC
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_FROM_BYTEARRAY
        QT_NO_FOREACH
        QT_NO_QPAIR
        QT_USE_NODISCARD_FILE_OPEN
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}
        ../../3rdparty/tinycbor/src
        ../shared
)
qt_internal_return_unless_building_tools()

qt_internal_extend_sbom(${target_name}
    COPYRIGHTS
        "Copyright (C) 2013 Olivier Goffart <ogoffart@woboq.com>"
        "Copyright (C) 2014 Olivier Goffart <ogoffart@woboq.org>"
        "Copyright (C) 2016 Intel Corporation."
        "Copyright (C) 2018 Intel Corporation."
        "Copyright (C) 2018 The Qt Company Ltd."
        "Copyright (C) 2019 Olivier Goffart <ogoffart@woboq.com>"
)

## Scopes:
#####################################################################

qt_internal_extend_target(${target_name} CONDITION MSVC
    DEFINES
        _CRT_SECURE_NO_WARNINGS
)
