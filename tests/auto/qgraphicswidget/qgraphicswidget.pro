load(qttest_p4)

QT += widgets widgets-private
QT += core-private gui-private

SOURCES  += tst_qgraphicswidget.cpp


mac*:CONFIG+=insignificant_test
