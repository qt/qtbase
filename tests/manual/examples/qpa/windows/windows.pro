CONFIG += console
QT += gui-private core-private

HEADERS += window.h
SOURCES += window.cpp main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qpa/windows
INSTALLS += target
