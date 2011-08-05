load(qttest_p4)

QT += core-private gui-private

SOURCES  += tst_qfontdialog.cpp

mac:!qpa {
    OBJECTIVE_SOURCES += tst_qfontdialog_mac_helpers.mm
    LIBS += -framework Cocoa
}

contains(QT_CONFIG,xcb):qpa:CONFIG+=insignificant_test  # QTBUG-20756 crashes on qpa, xcb
