SOURCES += semaphores.cpp
QT = core

CONFIG += cmdline

# install
target.path = $$[QT_INSTALL_EXAMPLES]/corelib/threads/semaphores
INSTALLS += target
