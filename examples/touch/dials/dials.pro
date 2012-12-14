QT += widgets

SOURCES += main.cpp
FORMS += dials.ui

# install
target.path = $$[QT_INSTALL_EXAMPLES]/touch/dials
INSTALLS += target

simulator: warning(This example might not fully work on Simulator platform)
