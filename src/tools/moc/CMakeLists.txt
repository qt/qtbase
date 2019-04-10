# Generated from moc.pro.

#####################################################################
## moc Tool:
#####################################################################

add_qt_tool(moc
    BOOTSTRAP
    TOOLS_TARGET Core # special case
    SOURCES
        cbordevice.h
        generator.cpp generator.h
        main.cpp  # special case
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
        QT_NO_COMPRESS
        QT_NO_FOREACH
    LIBRARIES
        tinycbor
    # CONFIG = "force_bootstrap"
    # QMAKE_TARGET_DESCRIPTION = "Qt Meta Object Compiler"
    # _LOADED = "qt_tool"
    # _OPTION = "host_build"
)
