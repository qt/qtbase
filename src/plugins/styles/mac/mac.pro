TARGET = qmacstyle

QT += widgets-private

SOURCES += \
    main.mm \
    qmacstyle_mac.mm

HEADERS += \
    qmacstyle_mac_p.h \
    qmacstyle_mac_p_p.h

LIBS_PRIVATE += -framework AppKit

DISTFILES += macstyle.json

PLUGIN_TYPE = styles
PLUGIN_CLASS_NAME = QMacStylePlugin
load(qt_plugin)
