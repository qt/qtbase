SOURCES   = main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/graphicsview/anchorlayout
sources.files = $$SOURCES $$HEADERS $$RESOURCES anchorlayout.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/graphicsview/anchorlayout
INSTALLS += target sources

TARGET = anchorlayout
QT += widgets
