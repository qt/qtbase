CONFIG += console
QT += gui-private core-private

HEADERS += window.h
SOURCES += window.cpp main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/qpa/windows
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS windows.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/qpa/windows
INSTALLS += target sources
