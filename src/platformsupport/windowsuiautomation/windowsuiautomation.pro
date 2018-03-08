TARGET = QtWindowsUIAutomationSupport
MODULE = windowsuiautomation_support

QT = core-private gui-private
CONFIG += static internal_module

HEADERS += \
    qwindowsuiawrapper_p.h \
    uiaattributeids_p.h \
    uiacontroltypeids_p.h \
    uiaerrorids_p.h \
    uiaeventids_p.h \
    uiageneralids_p.h \
    uiaserverinterfaces_p.h \
    uiaclientinterfaces_p.h \
    uiapatternids_p.h \
    uiapropertyids_p.h \
    uiatypes_p.h

SOURCES += \
    qwindowsuiawrapper.cpp

load(qt_module)
