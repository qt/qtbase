# Generated from generate.pro.

#####################################################################
## generate Binary:
#####################################################################

qt_add_executable(generate
    GUI
    SOURCES
        main.cpp
    INCLUDE_DIRECTORIES
        .
        /usr/include/freetype2
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
)
