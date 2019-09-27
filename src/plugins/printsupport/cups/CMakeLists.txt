# Generated from cups.pro.

qt_find_package(Cups PROVIDED_TARGETS Cups::Cups) # special case

#####################################################################
## cupsprintersupport Plugin:
#####################################################################

add_qt_plugin(cupsprintersupport
    TYPE printsupport
    CLASS_NAME QCupsPrinterSupportPlugin
    SOURCES
        main.cpp
        qcupsprintengine.cpp qcupsprintengine_p.h
        qcupsprintersupport.cpp qcupsprintersupport_p.h
        qppdprintdevice.cpp qppdprintdevice.h
    INCLUDE_DIRECTORIES
        ../../../printsupport/kernel
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::PrintSupport
        Qt::PrintSupportPrivate
        Cups::Cups
    # MODULE = "cupsprintersupport"
    # OTHER_FILES = "cups.json"
    # PLUGIN_CLASS_NAME = "QCupsPrinterSupportPlugin"
    # _LOADED = "qt_plugin"
)
