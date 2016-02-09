# Only headers here, no library is wanted.
TEMPLATE = subdirs
VERSION = $$MODULE_VERSION
MODULE_INCNAME = QtPlatformHeaders

include(nativecontexts/nativecontexts.pri)
include(xcbfunctions/xcbfunctions.pri)
include(eglfsfunctions/eglfsfunctions.pri)
include(windowsfunctions/windowsfunctions.pri)
include(helper/helper.pri)
include(cocoafunctions/cocoafunctions.pri)
include(waylandfunctions/waylandfunctions.pri)

QMAKE_DOCS = $$PWD/doc/qtplatformheaders.qdocconf

load(qt_module_headers)
load(qt_docs)
load(qt_installs)
