TARGET = qwindows

QT += \
    core-private gui-private \
    eventdispatcher_support-private \
    fontdatabase_support-private theme_support-private

qtHaveModule(platformcompositor_support-private): QT += platformcompositor_support-private

qtConfig(accessibility): QT += accessibility_support-private
qtConfig(vulkan): QT += vulkan_support-private

qtConfig(directwrite3): DEFINES *= QT_USE_DIRECTWRITE2 QT_USE_DIRECTWRITE3

LIBS += -ldwmapi
QMAKE_USE_PRIVATE += gdi32

include(windows.pri)

SOURCES +=  \
    main.cpp \
    qwindowsbackingstore.cpp \
    qwindowsgdiintegration.cpp \
    qwindowsgdinativeinterface.cpp

HEADERS +=  \
    qwindowsbackingstore.h \
    qwindowsgdiintegration.h \
    qwindowsgdinativeinterface.h

OTHER_FILES += windows.json

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QWindowsIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)
