#! [0]
HEADERS        = interfaces.h \
                 mainwindow.h \
                 paintarea.h \
                 plugindialog.h
SOURCES        = main.cpp \
                 mainwindow.cpp \
                 paintarea.cpp \
                 plugindialog.cpp
symbian {
    LIBS           = -lpnp_basictools.lib
} else {
    LIBS           = -L$${QT_BUILD_TREE}/examples/tools/plugandpaint/plugins -lpnp_basictools
}

if(!debug_and_release|build_pass):CONFIG(debug, debug|release) {
   mac:LIBS = $$member(LIBS, 0) $$member(LIBS, 1)_debug
   win32:LIBS = $$member(LIBS, 0) $$member(LIBS, 1)d
}
#! [0]

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/plugandpaint
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS plugandpaint.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/plugandpaint
INSTALLS += target sources

symbian: CONFIG += qt_example
QT += widgets
maemo5: CONFIG += qt_example

symbian: warning(This example might not fully work on Symbian platform)
maemo5: warning(This example might not fully work on Maemo platform)
simulator: warning(This example might not fully work on Simulator platform)
