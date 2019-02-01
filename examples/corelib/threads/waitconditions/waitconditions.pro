QT = core
CONFIG -= moc
CONFIG += cmdline

SOURCES += waitconditions.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/corelib/threads/waitconditions
INSTALLS += target
