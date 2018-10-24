# Generated from xml.pro.

#####################################################################
## QtXml Module:
#####################################################################

add_qt_module(Xml
    SOURCES
        dom/qdom.cpp dom/qdom.h
        qtxmlglobal.h
        sax/qxml.cpp sax/qxml.h sax/qxml_p.h
    DEFINES
        QT_NO_USING_NAMESPACE
        QT_NO_FOREACH
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES
        Qt::Core
)

## Scopes:
#####################################################################

### FIXME
# extend_target(Xml CONDITION (MSVC) AND (TEST_architecture_arch STREQUAL i386))
#     SOURCES
#     ### FIXME: QMAKE_LFLAGS += /BASE:0x61000000
# )

add_qt_docs(./doc/qtxml.qdocconf)
