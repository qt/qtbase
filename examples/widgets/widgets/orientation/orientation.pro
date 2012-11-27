#-------------------------------------------------
#
# Project created by QtCreator 2010-08-04T10:27:31
#
#-------------------------------------------------

QT       += core gui widgets

TARGET = orientation
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS += \
    portrait.ui \
    landscape.ui

RESOURCES += \
    images.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/widgets/orientation
INSTALLS += target

simulator: warning(This example might not fully work on Simulator platform)
