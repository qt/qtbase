add_qt_tool(qlalr
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
)
