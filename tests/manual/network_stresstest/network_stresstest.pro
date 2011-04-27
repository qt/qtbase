load(qttest_p4)

QT = core network

SOURCES  += tst_network_stresstest.cpp \
    minihttpserver.cpp

HEADERS += \
    minihttpserver.h

RESOURCES += wwwfiles.qrc
QMAKE_RESOURCE_FLAGS += -no-compress
