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
target.path = $$[QT_INSTALL_EXAMPLES]/tools/styleplugin
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS stylewindow.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/tools/styleplugin/stylewindow
INSTALLS += target sources

QT += widgets
