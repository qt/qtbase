QT += widgets

SOURCES += main.cpp
RESOURCES += weatheranchorlayout.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/graphicsview/weatheranchorlayout
INSTALLS += target

simulator: warning(This example might not fully work on Simulator platform)
