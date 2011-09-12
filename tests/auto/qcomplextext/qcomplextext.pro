load(qttest_p4)
QT += widgets widgets-private
QT += core-private gui-private
SOURCES  += tst_qcomplextext.cpp
INCLUDEPATH += $$QT_SOURCE_TREE/src/3rdparty/harfbuzz/src

CONFIG += insignificant_test # QTBUG-21402
