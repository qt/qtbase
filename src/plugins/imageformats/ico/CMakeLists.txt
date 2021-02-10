# Generated from ico.pro.

#####################################################################
## QICOPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QICOPlugin
    OUTPUT_NAME qico
    TYPE imageformats
    SOURCES
        main.cpp main.h
        qicohandler.cpp qicohandler.h
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
)

#### Keys ignored in scope 1:.:.:ico.pro:<TRUE>:
# OTHER_FILES = "ico.json"
