# DEFINES += QT_PEPPER_USE_PEPPER_FONT_ENGINE
SOURCES += $$PWD/qpepperpluginmain.cpp \
           $$PWD/qpeppermodule.cpp \
           $$PWD/qpepperinstance.cpp \
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
           $$PWD/qpepperjavascriptbridge.cpp \

HEADERS += $$PWD/qpepperhelpers.h \
           $$PWD/qpeppermodule.h \
           $$PWD/qpepperinstance.h \
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
           $$PWD/qpepperjavascriptbridge.h \

RESOURCES += $$PWD/../../../../lib/fonts/naclfonts.qrc \
             $$PWD/pepper.qrc

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

nacl-newlib {
    LIBS += -lerror_handling
}
