qt_internal_add_plugin(QIBaseDriverPlugin
    OUTPUT_NAME qsqlibase
    TYPE sqldrivers
    SOURCES
        main.cpp
        qsql_ibase.cpp qsql_ibase_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    PUBLIC_LIBRARIES
        Interbase::Interbase
        Qt::Core
        Qt::CorePrivate
        Qt::SqlPrivate)
