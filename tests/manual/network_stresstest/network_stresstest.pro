TARGET = tst_network_stresstest

QT = core-private network-private testlib

SOURCES  += tst_network_stresstest.cpp \
    minihttpserver.cpp

HEADERS += \
    minihttpserver.h

RESOURCES += wwwfiles.qrc
QMAKE_RESOURCE_FLAGS += -no-compress
LIBS += $$QMAKE_LIBS_NETWORK
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
