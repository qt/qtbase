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


simulator: warning(This example does not work on Simulator platform)
