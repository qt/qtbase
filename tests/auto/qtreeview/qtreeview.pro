load(qttest_p4)
SOURCES  += tst_qtreeview.cpp

contains(QT_CONFIG,xcb):qpa:CONFIG+=insignificant_test  # QTBUG-20756 crashes on qpa, xcb
