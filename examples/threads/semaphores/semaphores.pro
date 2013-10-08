SOURCES += semaphores.cpp
QT = core gui

CONFIG -= app_bundle
CONFIG += console

# install
target.path = $$[QT_INSTALL_EXAMPLES]/threads/semaphores
INSTALLS += target
