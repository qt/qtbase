TARGET = QtWindowsUIAutomationSupport
MODULE = windowsuiautomation_support

QT = core-private gui-private
CONFIG += static internal_module

HEADERS += \
    qwindowsuiawrapper_p.h \
    uiaattributeids.h \
    uiacontroltypeids.h \
    uiaerrorids.h \
    uiaeventids.h \
    uiageneralids.h \
    uiaserverinterfaces.h \
    uiaclientinterfaces.h \
    uiapatternids.h \
    uiapropertyids.h \
    uiatypes.h

SOURCES += \
    qwindowsuiawrapper.cpp

load(qt_module)
