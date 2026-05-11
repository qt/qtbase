# Generated from ohos.pro.

#####################################################################
## QOhosPrinterSupportPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QOhosPrinterSupportPlugin
    OUTPUT_NAME ohosprintsupport
    PLUGIN_TYPE printsupport
    EXCEPTIONS
    SOURCES
        main.cpp
        qohosnativeprint.cpp qohosnativeprint.h
        qohospdfprintengine.cpp qohospdfprintengine.h
        qohosprintdevice.cpp qohosprintdevice.h
        qohosprintersupport.cpp qohosprintersupport.h
    DEFINES
        NAPI_CPP_EXCEPTIONS
        NODE_ADDON_API_ENABLE_TYPE_CHECK_ON_AS
    INCLUDE_DIRECTORIES
        ${QT_SOURCE_TREE}/src/printsupport/kernel
    LIBRARIES
        NodeAddonApi::NodeAddonApi
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::PrintSupportPrivate
        ohprint
)

#### Keys ignored in scope 1:.:.:ohos.pro:<TRUE>:
# MODULE = "ohosprintsupport"
# OTHER_FILES = "ohos.json"
