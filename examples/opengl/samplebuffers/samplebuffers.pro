HEADERS += glwidget.h
SOURCES += glwidget.cpp main.cpp

QT += opengl widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/opengl/samplebuffers
sources.files = $$SOURCES $$HEADERS samplebuffers.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/opengl/samplebuffers
INSTALLS += target sources


simulator: warning(This example might not fully work on Simulator platform)
