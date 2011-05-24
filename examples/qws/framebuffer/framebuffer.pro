TEMPLATE = app
TARGET = framebuffer
CONFIG -= qt

SOURCES += main.c

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/qws/framebuffer
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS framebuffer.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/qws/framebuffer
INSTALLS += target sources
QT += widgets

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
maemo5: include($$QT_SOURCE_TREE/examples/maemo5pkgrules.pri)

symbian: warning(This example does not work on Symbian platform)
simulator: warning(This example does not work on Simulator platform)
