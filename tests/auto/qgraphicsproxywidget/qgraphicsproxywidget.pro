load(qttest_p4)

QT += widgets widgets-private
QT += core-private gui-private

SOURCES  += tst_qgraphicsproxywidget.cpp

contains(QT_CONFIG,xcb):qpa:CONFIG+=insignificant_test  # QTBUG-20756 crashes on qpa, xcb
