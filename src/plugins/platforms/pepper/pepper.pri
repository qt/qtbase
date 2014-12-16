# DEFINES += QT_PEPPER_USE_PEPPER_FONT_ENGINE

INCLUDEPATH += $$PWD

SOURCES += $$PWD/qpepperpluginmain.cpp \
           $$PWD/qpeppermodule_p.cpp \
           $$PWD/qpepperinstance_p.cpp \
           $$PWD/qpepperfontdatabase.cpp \
           $$PWD/qpepperscreen.cpp \
           $$PWD/qpeppereventtranslator.cpp \
           $$PWD/qpepperintegration.cpp \
           $$PWD/qpeppertheme.cpp \
           $$PWD/qpepperplatformwindow.cpp \
           $$PWD/qpepperbackingstore.cpp \
           $$PWD/qpepperfontengine.cpp \
           $$PWD/qpepperglcontext.cpp \
           $$PWD/qpepperhelpers.cpp \
           $$PWD/qpeppercompositor.cpp \
           $$PWD/qpeppereventdispatcher.cpp \
           $$PWD/qpeppercursor.cpp \
           $$PWD/qpepperservices.cpp \
           $$PWD/qpepperclipboard.cpp

HEADERS += $$PWD/qpepperhelpers.h \
           $$PWD/qpeppermodule_p.h \
           $$PWD/qpepperinstance_p.h \
           $$PWD/qpeppereventtranslator.h \
           $$PWD/qpepperintegration.h \
           $$PWD/qpeppertheme.h \
           $$PWD/qpepperplatformwindow.h \
           $$PWD/qpepperbackingstore.h \
           $$PWD/qpepperfontdatabase.h \
           $$PWD/qpepperscreen.h \
           $$PWD/qpepperfontengine.h \
           $$PWD/qpepperglcontext.h \
           $$PWD/qpepperhelpers.h \
           $$PWD/qpeppercompositor.h \
           $$PWD/qpeppereventdispatcher.h \
           $$PWD/qpeppercursor.h \
           $$PWD/qpepperservices.h \
           $$PWD/qpepperclipboard.h

RESOURCES += $$PWD/../../../../lib/fonts/naclfonts.qrc

OTHER_FILES += $$PWD/../../../../tools/nacldemoserver/check_browser.js \
               $$PWD/../../../../tools/nacldemoserver/qtnaclloader.js \
               $$PWD/qpepperhelpers.js \
               $$PWD/qpepperfileaccess.js \

# Normally we would do this:
# QT += platformsupport-private
# However this file (pepper.pri) is included in the QtGui build, 
# which craetes a dependency conflict: QtGui needs platformsupport
# but platformsupport is normally built after QtGui.
# Include the files we need directly:
include($$PWD/../../../platformsupport/fontdatabases/basic/basic.pri)
include($$PWD/../../../platformsupport/eventdispatchers/eventdispatchers.pri)
INCLUDEPATH += $$PWD/../../../platformsupport/eventdispatchers

LIBS += -lppapi -lppapi_cpp -lppapi_gles2

!pnacl:nacl-newlib {
    LIBS += -lerror_handling
}
