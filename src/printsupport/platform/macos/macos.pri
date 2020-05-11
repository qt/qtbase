SOURCES += \
    $$PWD/qprintengine_mac.mm \
    $$PWD/qpaintengine_mac.mm \
    $$PWD/qcocoaprintersupport.mm \
    $$PWD/qcocoaprintdevice.mm

HEADERS += \
    $$PWD/qcocoaprintersupport_p.h \
    $$PWD/qcocoaprintdevice_p.h \
    $$PWD/qprintengine_mac_p.h \
    $$PWD/qpaintengine_mac_p.h

LIBS += -framework ApplicationServices -lcups
