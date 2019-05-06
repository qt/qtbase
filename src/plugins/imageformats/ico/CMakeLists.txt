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
        Qt::GuiPrivate # special case
    PUBLIC_LIBRARIES # special case
        Qt::Gui # special case
)

#### Keys ignored in scope 1:.:.:ico.pro:<TRUE>:
# OTHER_FILES = "ico.json"
# PLUGIN_CLASS_NAME = "QICOPlugin"
# _LOADED = "qt_plugin"
