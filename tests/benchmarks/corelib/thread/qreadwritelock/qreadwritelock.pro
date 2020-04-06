TEMPLATE = app
CONFIG += benchmark
CONFIG += c++14  # for std::shared_timed_mutex
CONFIG += c++1z  # for std::shared_mutex
QT = core-private testlib

TARGET = tst_bench_qreadwritelock
SOURCES += tst_qreadwritelock.cpp
