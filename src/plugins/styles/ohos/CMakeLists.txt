# Generated from ohos.pro.

#####################################################################
## QOhosStylePlugin Plugin:
#####################################################################

qt_internal_add_plugin(QOhosStylePlugin
    OUTPUT_NAME qohosstyle
    PLUGIN_TYPE styles
    SOURCES
        main.cpp
        qohosstyle.cpp qohosstyle_p.h
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::WidgetsPrivate
)

#### Keys ignored in scope 1:.:.:ohos.pro:<TRUE>:
# DISTFILES = "ohosstyle.json"
