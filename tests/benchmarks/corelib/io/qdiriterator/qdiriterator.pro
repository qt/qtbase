TARGET = tst_bench_qdiriterator

QT = core testlib

CONFIG += release
# Enable c++17 support for std::filesystem
qtConfig(c++1z): CONFIG += c++17

SOURCES += main.cpp qfilesystemiterator.cpp
HEADERS += qfilesystemiterator.h
