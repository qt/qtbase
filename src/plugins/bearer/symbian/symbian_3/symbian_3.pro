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

        !contains(S60_VERSION, 3.2):!contains(S60_VERSION, 5.0) {
            DEFINES += OCC_FUNCTIONALITY_AVAILABLE
            LIBS += -lextendedconnpref
        }
    }
}

TARGET.UID3 = 0x20021319
