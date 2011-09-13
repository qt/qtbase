load(qttest_p4)
QT += widgets widgets-private
QT += core core-private gui gui-private
SOURCES  += tst_qstatictext.cpp

CONFIG += insignificant_test   # QTBUG-21290 - crashes on qpa, xcb
