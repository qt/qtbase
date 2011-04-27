load(qttest_p4)
SOURCES  += tst_platformsocketengine.cpp

include(../platformsocketengine/platformsocketengine.pri)

requires(contains(QT_CONFIG,private_tests))

MOC_DIR=tmp

QT = core network

symbian {
    TARGET.CAPABILITY = NetworkServices
    INCLUDEPATH += $$OS_LAYER_SYSTEMINCLUDE
    LIBS += -lesock
}
