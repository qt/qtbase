HEADERS         = mainwindow.h
SOURCES         = main.cpp \
                  mainwindow.cpp
RESOURCES       = dockwidgets.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/mainwindows/dockwidgets
sources.files = $$SOURCES $$HEADERS $$RESOURCES dockwidgets.pro images
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/mainwindows/dockwidgets
INSTALLS += target sources

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
