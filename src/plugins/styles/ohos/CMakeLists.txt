# Generated from ohos.pro.

#####################################################################
## QOhosStylePlugin Plugin:
#####################################################################

qt_internal_add_plugin(QOhosStylePlugin
    OUTPUT_NAME qohosstyle
    PLUGIN_TYPE styles
    EXCEPTIONS
    SOURCES
        main.cpp
        qohosstyle.cpp qohosstyle_p.h
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::WidgetsPrivate
        NodeAddonApi::NodeAddonApi
        ace_napi.z
    DEFINES
        NAPI_CPP_EXCEPTIONS
)

#### Keys ignored in scope 1:.:.:ohos.pro:<TRUE>:
# DISTFILES = "ohosstyle.json"
