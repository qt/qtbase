HEADERS       = mainwindow.h
SOURCES       = main.cpp \
                mainwindow.cpp
#! [0]
RESOURCES     = application.qrc
#! [0]

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/mainwindows/application
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS application.pro images
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/mainwindows/application
INSTALLS += target sources

symbian: CONFIG += qt_example
QT += widgets
