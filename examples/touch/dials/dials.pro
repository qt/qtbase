SOURCES += main.cpp
FORMS += dials.ui

# install
target.path = $$[QT_INSTALL_EXAMPLES]/touch/dials
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS dials.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/touch/dials
INSTALLS += target sources
QT += widgets


simulator: warning(This example might not fully work on Simulator platform)
