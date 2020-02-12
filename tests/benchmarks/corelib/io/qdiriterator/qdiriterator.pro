TARGET = tst_bench_qdiriterator

QT = core testlib

CONFIG += release
# Enable c++17 support for std::filesystem
qtConfig(cxx17_filesystem) {
    CONFIG += c++17
    gcc:lessThan(QMAKE_GCC_MAJOR_VERSION, 9): \
        QMAKE_LFLAGS += -lstdc++fs
}

SOURCES += main.cpp qfilesystemiterator.cpp
HEADERS += qfilesystemiterator.h
