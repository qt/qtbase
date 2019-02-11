# Generated from cups.pro.

find_package(Cups)

#####################################################################
## cupsprintersupport Plugin:
#####################################################################

add_qt_plugin(cupsprintersupport
    TYPE printsupport
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
