SOURCES += \
    $$PWD/qprintengine_mac.mm \
    $$PWD/qpaintengine_mac.mm \
    $$PWD/qcocoaprintdevice.mm

HEADERS += \
    $$PWD/qcocoaprintersupport_p.h \
    $$PWD/qcocoaprintdevice_p.h \
    $$PWD/qprintengine_mac_p.h \
    $$PWD/qpaintengine_mac_p.h

# Disable PCH to allow selectively enabling QT_STATICPLUGIN
NO_PCH_SOURCES += $$PWD/qcocoaprintersupport.mm

LIBS += -framework ApplicationServices -lcups

OTHER_FILES += cocoa.json
