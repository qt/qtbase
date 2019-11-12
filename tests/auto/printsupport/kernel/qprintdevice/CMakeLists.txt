# Generated from qprintdevice.pro.

#####################################################################
## tst_qprintdevice Test:
#####################################################################

add_qt_test(tst_qprintdevice
    SOURCES
        tst_qprintdevice.cpp
    DEFINES
        QT_USE_USING_NAMESPACE
    LIBRARIES
        Qt::PrintSupportPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::PrintSupport
)
