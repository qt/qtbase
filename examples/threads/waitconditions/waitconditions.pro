QT = core gui
CONFIG -= moc app_bundle
CONFIG += console

SOURCES += waitconditions.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/threads/waitconditions
INSTALLS += target

simulator: warning(This example might not fully work on Simulator platform)
