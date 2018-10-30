add_qt_tool(qdbuscpp2xml
    SOURCES
        ../moc/cbordevice.h
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
        ../moc/
        ../moc/../../3rdparty/tinycbor/src
    LIBRARIES
        Qt::CorePrivate
        Qt::DBusPrivate
)
