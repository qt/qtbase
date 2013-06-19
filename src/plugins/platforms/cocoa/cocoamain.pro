# Additional Qt project file for qtmain lib on OS X
!macx:error("$$_FILE_ is intended only for OS X!")

TEMPLATE = lib
TARGET = qtcocoamain
DESTDIR = $$QT.core.libs

CONFIG += staticlib release

QT = core-private gui-private
LIBS += -framework Cocoa

HEADERS = qcocoaintrospection.h

OBJECTIVE_SOURCES = qcocoamain.mm \
                    qcocoaintrospection.mm

load(qt_installs)
load(qt_targets)
