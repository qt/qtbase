SOURCES   = main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/graphicsview/anchorlayout
sources.files = $$SOURCES $$HEADERS $$RESOURCES anchorlayout.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/graphicsview/anchorlayout
INSTALLS += target sources

TARGET = anchorlayout

symbian: CONFIG += qt_example
maemo5: CONFIG += qt_example

simulator: warning(This example might not fully work on Simulator platform)
