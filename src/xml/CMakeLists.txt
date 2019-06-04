# Generated from xml.pro.

#####################################################################
## Xml Module:
#####################################################################

add_qt_module(Xml
    SOURCES
        dom/qdom.cpp dom/qdom.h
        qtxmlglobal.h
        sax/qxml.cpp sax/qxml.h sax/qxml_p.h
    DEFINES
        QT_NO_FOREACH
        QT_NO_USING_NAMESPACE
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES
        Qt::Core
)

#### Keys ignored in scope 1:.:.:xml.pro:<TRUE>:
# PRECOMPILED_HEADER = <EMPTY>
# _LOADED = "qt_module"

## Scopes:
#####################################################################

extend_target(Xml CONDITION MSVC AND (TEST_architecture_arch STREQUAL "i386")
    LINK_OPTIONS
        "/BASE:0x61000000"
)
add_qt_docs(
    doc/qtxml.qdocconf
)

