# Generated from qdbusxmlparser.pro.

#####################################################################
## tst_qdbusxmlparser Test:
#####################################################################

add_qt_test(tst_qdbusxmlparser
    SOURCES
        tst_qdbusxmlparser.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::DBusPrivate
    PUBLIC_LIBRARIES
        Qt::DBus
        Qt::Xml
)
