# Generated from ico.pro.

#####################################################################
## qico Plugin:
#####################################################################

add_qt_plugin(qico
    TYPE imageformats
    SOURCES
        main.cpp main.h
        qicohandler.cpp qicohandler.h
    LIBRARIES
        Qt::Gui
        Qt::CorePrivate
)
