# Generated from tuiotouch.pro.

#####################################################################
## QTuioTouchPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QTuioTouchPlugin
    OUTPUT_NAME qtuiotouchplugin
    PLUGIN_TYPE generic
    DEFAULT_IF FALSE
    SOURCES
        main.cpp
        qoscbundle.cpp qoscbundle_p.h
        qoscmessage.cpp qoscmessage_p.h
        qtuiocursor_p.h
        qtuiohandler.cpp qtuiohandler_p.h
        qtuiotoken_p.h
    DEFINES
        QT_NO_FOREACH
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Network
)

#### Keys ignored in scope 1:.:.:tuiotouch.pro:<TRUE>:
# OTHER_FILES = "tuiotouch.json"
# PLUGIN_EXTENDS = "-"
