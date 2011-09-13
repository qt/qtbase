TARGET = qopenwf

load(qt_plugin)
QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/platforms

QT += core-private gui-private platformsupport-private

CONFIG += qpa/genericunixfontdatabase

HEADERS += \
    qopenwfddevice.h \
    qopenwfdintegration.h \
    qopenwfdnativeinterface.h \
    qopenwfdscreen.h \
    qopenwfdbackingstore.h \
    qopenwfdevent.h \
    qopenwfdglcontext.h \
    qopenwfdoutputbuffer.h \
    qopenwfdport.h \
    qopenwfdwindow.h \
    qopenwfdportmode.h

SOURCES += \
    main.cpp \
    qopenwfddevice.cpp \
    qopenwfdintegration.cpp \
    qopenwfdnativeinterface.cpp \
    qopenwfdscreen.cpp \
    qopenwfdbackingstore.cpp \
    qopenwfdevent.cpp \
    qopenwfdglcontext.cpp \
    qopenwfdoutputbuffer.cpp \
    qopenwfdport.cpp \
    qopenwfdportmode.cpp \
    qopenwfdwindow.cpp

LIBS += -lWFD -lgbm -lGLESv2 -lEGL

target.path += $$[QT_INSTALL_PLUGINS]/platforms
INSTALLS += target

