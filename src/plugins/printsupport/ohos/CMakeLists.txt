# Generated from ohos.pro.

#####################################################################
## QOhosPrinterSupportPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QOhosPrinterSupportPlugin
    OUTPUT_NAME ohosprintsupport
    PLUGIN_TYPE printsupport
    SOURCES
        main.cpp
        qohosnativeprint.cpp qohosnativeprint.h
        qohospdfprintengine.cpp qohospdfprintengine.h
        qohosprintdevice.cpp qohosprintdevice.h
        qohosprintersupport.cpp qohosprintersupport.h
    INCLUDE_DIRECTORIES
        ${QT_SOURCE_TREE}/src/printsupport/kernel
    LIBRARIES
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
