# Generated from qabstractfileengine.pro.

#####################################################################
## tst_qabstractfileengine Test:
#####################################################################

add_qt_test(tst_qabstractfileengine
    SOURCES
        tst_qabstractfileengine.cpp
    LIBRARIES
        Qt::CorePrivate
)

# Resources:
set(qabstractfileengine_resource_files
    "resources/"
)

add_qt_resource(tst_qabstractfileengine "qabstractfileengine"
    PREFIX
        "/tst_qabstractfileengine/"
    FILES
        ${qabstractfileengine_resource_files}
)

