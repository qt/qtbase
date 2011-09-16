load(qttest_p4)
macx:CONFIG -= app_bundle

SOURCES += tst_v8.cpp v8test.cpp
HEADERS += v8test.h

CONFIG += parallel_test

QT = core v8-private
