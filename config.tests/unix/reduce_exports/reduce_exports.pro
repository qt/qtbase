TEMPLATE = lib
CONFIG += dll hide_symbols
SOURCES = fvisibility.c

isEmpty(QMAKE_CFLAGS_HIDESYMS): error("Nope")
