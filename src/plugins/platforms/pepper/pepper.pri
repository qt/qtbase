# DEFINES += QT_PEPPER_USE_PEPPER_FONT_ENGINE
# DEFINES += QT_PEPPER_RUN_QT_ON_THREAD
# DEFINES += QT_PEPPER_USE_QT_MESSAGE_LOOP

INCLUDEPATH += $$PWD

HEADERS += \
           $$PWD/qpepperbackingstore.h \
           $$PWD/qpepperclipboard.h \
           $$PWD/qpeppercompositor.h \
           $$PWD/qpeppercursor.h \
           $$PWD/qpeppereventdispatcher.h \
           $$PWD/qpeppereventtranslator.h \
           $$PWD/qpepperfontdatabase.h \
           $$PWD/qpepperfontengine.h \
           $$PWD/qpepperglcontext.h \
           $$PWD/qpepperhelpers.h \
           $$PWD/qpepperhelpers.h \
           $$PWD/qpepperinstance_p.h \
           $$PWD/qpepperintegration.h \
           $$PWD/qpeppermodule_p.h \
           $$PWD/qpepperwindow.h \
           $$PWD/qpepperscreen.h \
           $$PWD/qpepperservices.h \
           $$PWD/qpeppertheme.h \

SOURCES += \
           $$PWD/qpepperbackingstore.cpp \
           $$PWD/qpepperclipboard.cpp \
           $$PWD/qpeppercompositor.cpp \
           $$PWD/qpeppercursor.cpp \
           $$PWD/qpeppereventdispatcher.cpp \
           $$PWD/qpeppereventtranslator.cpp \
           $$PWD/qpepperfontdatabase.cpp \
           $$PWD/qpepperfontengine.cpp \
           $$PWD/qpepperglcontext.cpp \
           $$PWD/qpepperhelpers.cpp \
           $$PWD/qpepperinstance_p.cpp \
           $$PWD/qpepperintegration.cpp \
           $$PWD/qpeppermodule_p.cpp \
           $$PWD/qpepperpluginmain.cpp \
           $$PWD/qpepperscreen.cpp \
           $$PWD/qpepperservices.cpp \
           $$PWD/qpeppertheme.cpp \
           $$PWD/qpepperwindow.cpp \

RESOURCES += $$PWD/../../../../lib/fonts/naclfonts.qrc

OTHER_FILES += \
               $$PWD/../../../../tools/nacldemoserver/check_browser.js \
               $$PWD/../../../../tools/nacldemoserver/qtnaclloader.js \
               $$PWD/qpepperfileaccess.js \
               $$PWD/qpepperhelpers.js \

# Normally we would do this:
# QT += platformsupport-private
# However this file (pepper.pri) is included in the QtGui build, 
# which craetes a dependency conflict: QtGui needs platformsupport
# but platformsupport is normally built after QtGui.
# Include the files we need directly:
include($$PWD/../../../platformsupport/eventdispatchers/eventdispatchers.pri)
include($$PWD/../../../platformsupport/fontdatabases/basic/basic.pri)
INCLUDEPATH += $$PWD/../../../platformsupport/eventdispatchers

LIBS += -lppapi -lppapi_cpp -lppapi_gles2

!pnacl:nacl-newlib {
    LIBS += -lerror_handling
}
