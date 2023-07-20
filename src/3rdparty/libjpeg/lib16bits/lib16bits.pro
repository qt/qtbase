TARGET = qtlibjpeg16bits

include($$PWD/../common.pri)

DEFINES += BITS_IN_JSAMPLE=16

SOURCES = $$JPEG16_SOURCES
