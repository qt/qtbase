SOURCES += semaphores.cpp
QT = core gui

CONFIG -= app_bundle
CONFIG += console

# install
target.path = $$[QT_INSTALL_EXAMPLES]/threads/semaphores
INSTALLS += target


simulator: warning(This example might not fully work on Simulator platform)
