QT += widgets

HEADERS = knob.h
SOURCES = main.cpp knob.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/touch/knobs
INSTALLS += target

simulator: warning(This example might not fully work on Simulator platform)
