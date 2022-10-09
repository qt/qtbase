
qt_find_package(WrapCPDB
                PROVIDED_TARGETS WrapCPDB::WrapCPDB MODULE_NAME printsupport QMAKE_LIB cpdb)

qt_internal_add_plugin(QCpdbPrinterSupportPlugin
    OUTPUT_NAME cpdbprintersupport
    PLUGIN_TYPE printsupport
    SOURCES
        main.cpp
        qcpdbprintengine.cpp qcpdbprintengine_p.h
        qcpdbprintersupport.cpp qcpdbprintersupport_p.h
        qcpdbprintdevice.cpp qcpdbprintdevice.h
    INCLUDE_DIRECTORIES
        ../../../printsupport/kernel
    LIBRARIES
        WrapCPDB::WrapCPDB
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::PrintSupport
        Qt::PrintSupportPrivate
)
