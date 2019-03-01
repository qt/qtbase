# Generated from rcc.pro.

#####################################################################
## rcc Tool:
#####################################################################

add_qt_tool(rcc
    BOOTSTRAP
    SOURCES
        main.cpp
        rcc.cpp rcc.h
    DEFINES
        QT_RCC
        QT_NO_CAST_FROM_ASCII
        QT_NO_FOREACH
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}
    # CONFIG = "force_bootstrap"
    # QMAKE_TARGET_DESCRIPTION = "Qt Resource Compiler"
    # _LOADED = "qt_tool"
    # _OPTION = "host_build"
)
