load(qttest_p4)
SOURCES  += tst_qmenu.cpp

qpa:contains(QT_CONFIG,xcb):CONFIG+=insignificant_test  # QTBUG-21100, unstably fails
