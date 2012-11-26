QT += opengl svg widgets

HEADERS += glwidget.h
SOURCES += glwidget.cpp main.cpp
RESOURCES += pbuffers2.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/opengl/pbuffers2
sources.files = $$SOURCES $$HEADERS $$RESOURCES pbuffers2.pro *.png *.svg
sources.path = $$[QT_INSTALL_EXAMPLES]/opengl/pbuffers2
INSTALLS += target sources

simulator: warning(This example might not fully work on Simulator platform)
