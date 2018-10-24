# FIXME Add the rest of the stuff

add_qt_module(PlatformHeaders SOURCES fake.cpp)

extend_target(PlatformHeaders CONDITION APPLE SOURCES cocoafunctions/qcocoawindowfunctions.h)

add_qt_docs(./doc/qtplatformheaders.qdocconf)
