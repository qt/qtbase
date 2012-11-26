HEADERS       = mainwindow.h
SOURCES       = main.cpp \
                mainwindow.cpp
#! [0]
RESOURCES     = application.qrc
#! [0]

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/mainwindows/application
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS application.pro images
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/mainwindows/application
INSTALLS += target sources

QT += widgets

simulator: warning(This example might not fully work on Simulator platform)
