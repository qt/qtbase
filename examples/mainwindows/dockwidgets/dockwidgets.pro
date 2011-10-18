HEADERS         = mainwindow.h
SOURCES         = main.cpp \
                  mainwindow.cpp
RESOURCES       = dockwidgets.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/mainwindows/dockwidgets
sources.files = $$SOURCES $$HEADERS $$RESOURCES dockwidgets.pro images
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/mainwindows/dockwidgets
INSTALLS += target sources

QT += widgets printsupport

simulator: warning(This example might not fully work on Simulator platform)
