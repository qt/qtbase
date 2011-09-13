HEADERS       = mainwindow.h
SOURCES       = main.cpp \
                mainwindow.cpp
RESOURCES     = sdi.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/mainwindows/sdi
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS sdi.pro images
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/mainwindows/sdi
INSTALLS += target sources

symbian: CONFIG += qt_example
QT += widgets
maemo5: CONFIG += qt_example

symbian: warning(This example might not fully work on Symbian platform)
simulator: warning(This example might not fully work on Simulator platform)
