# Generated from xml.pro.

#####################################################################
## Xml Module:
#####################################################################

qt_internal_add_module(Xml
    SOURCES
        dom/qdom.cpp dom/qdom.h dom/qdom_p.h
        dom/qdomhelpers.cpp dom/qdomhelpers_p.h
        qtxmlglobal.h
    DEFINES
        QT_NO_FOREACH
        QT_NO_USING_NAMESPACE
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES
        Qt::Core
    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(Xml CONDITION MSVC AND (TEST_architecture_arch STREQUAL "i386")
    LINK_OPTIONS
        "/BASE:0x61000000"
)
qt_internal_add_docs(Xml
    doc/qtxml.qdocconf
)

