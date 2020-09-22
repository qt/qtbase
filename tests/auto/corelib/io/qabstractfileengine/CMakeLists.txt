# Generated from qabstractfileengine.pro.

#####################################################################
## tst_qabstractfileengine Test:
#####################################################################

qt_internal_add_test(tst_qabstractfileengine
    SOURCES
        tst_qabstractfileengine.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
)

# Resources:
set(qabstractfileengine_resource_files
    "resources/"
)

qt_internal_add_resource(tst_qabstractfileengine "qabstractfileengine"
    PREFIX
        "/tst_qabstractfileengine/"
    FILES
        ${qabstractfileengine_resource_files}
)

