QT += concurrent widgets
CONFIG += console

SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtconcurrent/progressdialog
INSTALLS += target

simulator: warning(This example does not work on Simulator platform)
