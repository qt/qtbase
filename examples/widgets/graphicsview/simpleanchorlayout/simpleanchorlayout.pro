SOURCES   = main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/graphicsview/simpleanchorlayout
sources.files = $$SOURCES $$HEADERS $$RESOURCES simpleanchorlayout.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/graphicsview/simpleanchorlayout
INSTALLS += target sources

TARGET = simpleanchorlayout
QT += widgets

simulator: warning(This example might not fully work on Simulator platform)
