HEADERS         = mainwindow.h
SOURCES         = main.cpp \
                  mainwindow.cpp
RESOURCES       = dockwidgets.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/mainwindows/dockwidgets
sources.files = $$SOURCES $$HEADERS $$RESOURCES dockwidgets.pro images
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/mainwindows/dockwidgets
INSTALLS += target sources

symbian: CONFIG += qt_example
QT += widgets printsupport
maemo5: CONFIG += qt_example

symbian: warning(This example might not fully work on Symbian platform)
maemo5: warning(This example might not fully work on Maemo platform)
simulator: warning(This example might not fully work on Simulator platform)
