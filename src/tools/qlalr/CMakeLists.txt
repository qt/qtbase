# Generated from qlalr.pro.

#####################################################################
## qlalr Tool:
#####################################################################

add_qt_tool(qlalr
    TOOLS_TARGET Core # special case
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
    # OTHER_FILES = "lalr.g"
    # QMAKE_TARGET_DESCRIPTION = "Qt Look Ahead LR Parser Generator"
    # _LOADED = "qt_tool"
    # _OPTION = "host_build"
)
