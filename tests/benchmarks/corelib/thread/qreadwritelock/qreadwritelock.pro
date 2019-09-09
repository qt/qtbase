TEMPLATE = app
TARGET = tst_bench_qreadwritelock
QT = core-private testlib
SOURCES += tst_qreadwritelock.cpp
CONFIG += c++14  # for std::shared_timed_mutex
CONFIG += c++1z  # for std::shared_mutex

