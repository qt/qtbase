TEMPLATE = lib

TARGET = xml_snippets

#! [qmake_use]
QT += xml
#! [qmake_use]

load(qt_common)

QT += core xml

SOURCES += code/src_xml_dom_qdom.cpp
