HEADERS += glwidget.h
SOURCES += glwidget.cpp main.cpp

QT += opengl widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/opengl/samplebuffers
sources.files = $$SOURCES $$HEADERS samplebuffers.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/opengl/samplebuffers
INSTALLS += target sources

symbian: CONFIG += qt_example
maemo5: CONFIG += qt_example

symbian: warning(This example might not fully work on Symbian platform)
maemo5: warning(This example does not work on Maemo platform)
simulator: warning(This example might not fully work on Simulator platform)
