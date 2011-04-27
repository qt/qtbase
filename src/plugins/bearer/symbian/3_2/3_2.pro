include(../symbian.pri)

symbian {
    contains(S60_VERSION, 3.1) {
        is_using_gnupoc {
            LIBS += -lapengine
        } else {
            LIBS += -lAPEngine
        }
    } else {
        DEFINES += SNAP_FUNCTIONALITY_AVAILABLE
        LIBS += -lcmmanager
    }
}

TARGET = $${TARGET}_3_2
TARGET.UID3 = 0x2002131D
