TARGET = QtPlatformHeaders
CONFIG += header_module

include(xcbfunctions/xcbfunctions.pri)
include(eglfsfunctions/eglfsfunctions.pri)
include(windowsfunctions/windowsfunctions.pri)
include(helper/helper.pri)
include(cocoafunctions/cocoafunctions.pri)
include(waylandfunctions/waylandfunctions.pri)
include(linuxfbfunctions/linuxfbfunctions.pri)

QMAKE_DOCS = $$PWD/doc/qtplatformheaders.qdocconf

load(qt_module)
