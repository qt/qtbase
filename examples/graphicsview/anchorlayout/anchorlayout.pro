SOURCES   = main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/graphicsview/anchorlayout
sources.files = $$SOURCES $$HEADERS $$RESOURCES anchorlayout.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/graphicsview/anchorlayout
INSTALLS += target sources

TARGET = anchorlayout
QT += widgets

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
maemo5: include($$QT_SOURCE_TREE/examples/maemo5pkgrules.pri)

simulator: warning(This example might not fully work on Simulator platform)
