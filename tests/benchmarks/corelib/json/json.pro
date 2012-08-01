TARGET = tst_bench_qtbinaryjson
QT = core testlib
CONFIG -= app_bundle

SOURCES += tst_bench_qtbinaryjson.cpp

TESTDATA = numbers.json test.json
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
