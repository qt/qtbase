# Generated from gif.pro.

#####################################################################
## qgif Plugin:
#####################################################################

add_qt_plugin(qgif
    TYPE imageformats
    SOURCES
        main.cpp main.h
        qgifhandler.cpp qgifhandler_p.h
    LIBRARIES
        Qt::Gui
)
