HEADERS    = stylewindow.h
SOURCES    = stylewindow.cpp \
             main.cpp

TARGET     = styleplugin
win32 {
    debug:DESTDIR = ../debug/
    release:DESTDIR = ../release/
} else {
    DESTDIR    = ../
}

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/styleplugin
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS stylewindow.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/styleplugin/stylewindow
INSTALLS += target sources

symbian: CONFIG += qt_example
QT += widgets
maemo5: CONFIG += qt_example
