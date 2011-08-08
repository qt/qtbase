load(qttest_p4)
SOURCES  += tst_qgraphicsgridlayout.cpp
CONFIG += parallel_test
contains(QT_CONFIG,xcb):qpa:CONFIG+=insignificant_test  # QTBUG-20756 crashes on qpa, xcb
