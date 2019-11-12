# Generated from gif.pro.

#####################################################################
## qgif Plugin:
#####################################################################

add_qt_plugin(qgif
    TYPE imageformats
    CLASS_NAME QGifPlugin
    SOURCES
        main.cpp main.h
        qgifhandler.cpp qgifhandler_p.h
    LIBRARIES # special case
        Qt::GuiPrivate # special case
    PUBLIC_LIBRARIES # special case
        Qt::Gui # special case
)

#### Keys ignored in scope 1:.:.:gif.pro:<TRUE>:
# OTHER_FILES = "gif.json"
