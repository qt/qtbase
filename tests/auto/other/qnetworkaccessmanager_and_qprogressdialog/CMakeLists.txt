# Generated from qnetworkaccessmanager_and_qprogressdialog.pro.

#####################################################################
## tst_qnetworkaccessmanager_and_qprogressdialog Test:
#####################################################################

qt_internal_add_test(tst_qnetworkaccessmanager_and_qprogressdialog
    SOURCES
        tst_qnetworkaccessmanager_and_qprogressdialog.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Network
        Qt::Widgets
    QT_TEST_SERVER_LIST "apache2"
)
