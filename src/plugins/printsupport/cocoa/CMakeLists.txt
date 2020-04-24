# Generated from cocoa.pro.

#####################################################################
## QCocoaPrinterSupportPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QCocoaPrinterSupportPlugin
    OUTPUT_NAME cocoaprintersupport
    TYPE printsupport
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        ${FWAppKit}
        Qt::Core
        Qt::Gui
        Qt::GuiPrivate
        Qt::PrintSupportPrivate
)

#### Keys ignored in scope 1:.:.:cocoa.pro:<TRUE>:
# MODULE = "cocoaprintersupport"
# OTHER_FILES = "cocoa.json"
