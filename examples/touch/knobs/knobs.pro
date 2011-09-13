HEADERS = knob.h
SOURCES = main.cpp knob.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/touch/knobs
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS knobs.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/touch/knobs
INSTALLS += target sources
QT += widgets

symbian: CONFIG += qt_example
maemo5: CONFIG += qt_example

symbian: warning(This example might not fully work on Symbian platform)
maemo5: warning(This example might not fully work on Maemo platform)
simulator: warning(This example might not fully work on Simulator platform)
