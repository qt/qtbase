QT = core testlib
CONFIG += benchmark
CONFIG -= app_bundle

TARGET = tst_bench_qtjson
SOURCES += tst_bench_qtjson.cpp

TESTDATA = numbers.json test.json
