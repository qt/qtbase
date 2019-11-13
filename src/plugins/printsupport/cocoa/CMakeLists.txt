# Generated from cocoa.pro.

#####################################################################
## cocoaprintersupport Plugin:
#####################################################################

qt_add_plugin(cocoaprintersupport
    TYPE printsupport
    CLASS_NAME QCocoaPrinterSupportPlugin
    SOURCES
        main.cpp
    LIBRARIES
        Qt::GuiPrivate
        Qt::PrintSupportPrivate
    PUBLIC_LIBRARIES
        ${FWAppKit}
        Qt::Core
        Qt::Gui
        Qt::PrintSupport
)

#### Keys ignored in scope 1:.:.:cocoa.pro:<TRUE>:
# MODULE = "cocoaprintersupport"
# OTHER_FILES = "cocoa.json"
