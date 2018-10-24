add_qt_tool(moc BOOTSTRAP
    LIBRARIES tinycbor
    SOURCES
        moc.cpp
        preprocessor.cpp
        generator.cpp
        parser.cpp
        token.cpp
        main.cpp
)
