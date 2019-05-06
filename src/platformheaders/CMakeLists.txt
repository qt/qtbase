# Generated from platformheaders.pro.

# special case begin

# FIXME Add the rest of the stuff

add_qt_module(PlatformHeaders)

extend_target(PlatformHeaders CONDITION APPLE SOURCES cocoafunctions/qcocoawindowfunctions.h)

add_qt_docs(
    doc/qtplatformheaders.qdocconf
)

# special case end
