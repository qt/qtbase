TEMPLATE = app
TARGET = tst_bench_qreadwritelock
QT = core testlib
SOURCES += tst_qreadwritelock.cpp
CONFIG += c++14  # for std::shared_timed_mutex

