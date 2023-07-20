TARGET = qtlibjpeg12bits

include($$PWD/../common.pri)

DEFINES += BITS_IN_JSAMPLE=12

SOURCES = $$JPEG12_SOURCES
