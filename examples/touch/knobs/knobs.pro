QT += widgets

HEADERS = knob.h
SOURCES = main.cpp knob.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/touch/knobs
INSTALLS += target
