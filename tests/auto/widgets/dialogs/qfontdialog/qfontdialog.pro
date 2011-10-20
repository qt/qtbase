load(qttest_p4)

QT += widgets widgets-private
QT += core-private gui-private

SOURCES  += tst_qfontdialog.cpp

mac:!qpa {
    OBJECTIVE_SOURCES += tst_qfontdialog_mac_helpers.mm
    LIBS += -framework Cocoa
}

