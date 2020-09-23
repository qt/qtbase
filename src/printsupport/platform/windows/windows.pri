SOURCES += \
    $$PWD/qprintengine_win.cpp \
    $$PWD/qwindowsprintdevice.cpp

# Disable PCH to allow selectively enabling QT_STATICPLUGIN
NO_PCH_SOURCES += $$PWD/qwindowsprintersupport.cpp

HEADERS += \
    $$PWD/qprintengine_win_p.h \
    $$PWD/qwindowsprintersupport_p.h \
    $$PWD/qwindowsprintdevice_p.h

OTHER_FILES += $$PWD/windows.json

LIBS += -lwinspool -lcomdlg32
QMAKE_USE_PRIVATE += user32 gdi32
