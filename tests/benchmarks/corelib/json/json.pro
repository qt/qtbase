QT = core testlib
CONFIG += benchmark
CONFIG -= app_bundle

TARGET = tst_bench_qtbinaryjson
SOURCES += tst_bench_qtbinaryjson.cpp

TESTDATA = numbers.json test.json
