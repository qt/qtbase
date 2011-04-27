include(../symbian.pri)

is_using_gnupoc {
    LIBS += -lapengine
} else {
    LIBS += -lAPEngine
}
TARGET = $${TARGET}_3_1
TARGET.UID3 = 0x2002131C
