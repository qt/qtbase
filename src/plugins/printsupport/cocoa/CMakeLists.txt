# Generated from cocoa.pro.

#####################################################################
## QCocoaPrinterSupportPlugin Plugin:
#####################################################################

qt_add_plugin(QCocoaPrinterSupportPlugin
    OUTPUT_NAME cocoaprintersupport
    TYPE printsupport
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
