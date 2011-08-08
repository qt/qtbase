load(qttest_p4)

QT += core-private gui-private

SOURCES  += tst_qgraphicswidget.cpp


mac*:CONFIG+=insignificant_test
qpa:contains(QT_CONFIG,xcb):CONFIG+=insignificant_test  # QTBUG-20778 unstable on qpa, xcb
