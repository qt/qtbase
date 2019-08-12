# Generated from qabstractfileengine.pro.

#####################################################################
## tst_qabstractfileengine Test:
#####################################################################

add_qt_test(tst_qabstractfileengine
    GUI
    SOURCES
        tst_qabstractfileengine.cpp
    LIBRARIES
        Qt::CorePrivate
)

# Resources:
add_qt_resource(tst_qabstractfileengine "qabstractfileengine"
    PREFIX
        "/tst_qabstractfileengine/"
    FILES
        "resources/"
)

