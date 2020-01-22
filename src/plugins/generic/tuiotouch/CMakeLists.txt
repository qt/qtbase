# Generated from tuiotouch.pro.

#####################################################################
## QTuioTouchPlugin Plugin:
#####################################################################

qt_add_plugin(QTuioTouchPlugin
    OUTPUT_NAME qtuiotouchplugin
    TYPE generic
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
        Qt::CorePrivate
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Network
)

#### Keys ignored in scope 1:.:.:tuiotouch.pro:<TRUE>:
# OTHER_FILES = "tuiotouch.json"
# PLUGIN_EXTENDS = "-"
