QT += widgets

SOURCES += main.cpp
RESOURCES += weatheranchorlayout.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/graphicsview/weatheranchorlayout
sources.files = $$SOURCES $$HEADERS $$RESOURCES weatheranchorlayout.pro images
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/graphicsview/weatheranchorlayout
INSTALLS += target sources

simulator: warning(This example might not fully work on Simulator platform)
