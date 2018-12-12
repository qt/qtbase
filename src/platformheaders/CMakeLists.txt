# FIXME Add the rest of the stuff

add_qt_module(PlatformHeaders SOURCES fake.cpp)
set_source_files_properties(fake.cpp PROPERTIES
    SKIP_AUTOMOC ON
    SKIP_AUTOUIC ON
    SKIP_AUTORCC ON
)

extend_target(PlatformHeaders CONDITION APPLE SOURCES cocoafunctions/qcocoawindowfunctions.h)

add_qt_docs(./doc/qtplatformheaders.qdocconf)
