# Generated from generate.pro.

#####################################################################
## generate Binary:
#####################################################################

add_qt_executable(generate
    GUI
    SOURCES
        main.cpp
    INCLUDE_DIRECTORIES
        .
        /usr/include/freetype2
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES
        Qt::Gui
)
