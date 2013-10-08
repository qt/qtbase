SOURCES += semaphores.cpp
QT = core

CONFIG -= app_bundle
CONFIG += console

# install
target.path = $$[QT_INSTALL_EXAMPLES]/threads/semaphores
INSTALLS += target
