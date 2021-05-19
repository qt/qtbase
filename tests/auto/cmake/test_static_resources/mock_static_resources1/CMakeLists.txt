qt_internal_add_module(MockStaticResources1
    STATIC
    PLUGIN_TYPES mockstaticresources
    SOURCES
        dummy.cpp
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES
        Qt::Core
)

qt_internal_add_resource(MockStaticResources1 "teststaticmodule1"
    PREFIX
        "/teststaticmodule1"
    FILES
        "testfile1.txt"
)
