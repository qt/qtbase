QT += widgets network

TARGET = secureudpclient
TEMPLATE = app

SOURCES += \
        main.cpp \
        association.cpp \
        mainwindow.cpp \
        addressdialog.cpp

HEADERS += \
        association.h \
        mainwindow.h \
        addressdialog.h

FORMS += \
        mainwindow.ui \
        addressdialog.ui

target.path = $$[QT_INSTALL_EXAMPLES]/network/secureudpclient
INSTALLS += target
