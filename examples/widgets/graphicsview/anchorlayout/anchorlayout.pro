SOURCES   = main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/graphicsview/anchorlayout
INSTALLS += target

TARGET = anchorlayout
QT += widgets


simulator: warning(This example might not fully work on Simulator platform)
