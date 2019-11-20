
# Generated from qdbusabstractadaptor.pro.

add_subdirectory(qmyserver)

add_qt_test(tst_qdbusabstractadaptor
    SOURCES
        myobject.h
        tst_qdbusabstractadaptor.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::DBus
)

