# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qlalr Tool:
#####################################################################

qt_get_tool_target_name(target_name qlalr)
qt_internal_add_tool(${target_name}
    TARGET_DESCRIPTION "Qt Look Ahead LR Parser Generator"
    INSTALL_DIR "${INSTALL_LIBEXECDIR}"
    TOOLS_TARGET Core
    SOURCES
        compress.cpp compress.h
        cppgenerator.cpp cppgenerator.h
        dotgraph.cpp dotgraph.h
        grammar.cpp grammar_p.h
        lalr.cpp lalr.h
        main.cpp
        parsetable.cpp parsetable.h
        recognizer.cpp recognizer.h
    DEFINES
        QT_NO_FOREACH
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
)
qt_internal_return_unless_building_tools()
