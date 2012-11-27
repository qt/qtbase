QT = core network
CONFIG += console
CONFIG -= app_bundle

SOURCES += main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/network/download
INSTALLS += target

simulator: warning(This example might not fully work on Simulator platform)
