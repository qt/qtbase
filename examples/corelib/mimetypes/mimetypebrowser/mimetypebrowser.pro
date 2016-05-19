TEMPLATE = app
QT += widgets
CONFIG -= app_bundle
CONFIG += c++11

SOURCES += \
    main.cpp \
    mimetypemodel.cpp \
    mainwindow.cpp

HEADERS += \
    mimetypemodel.h \
    mainwindow.h

target.path = $$[QT_INSTALL_EXAMPLES]/corelib/mimetypes/mimetypebrowser
INSTALLS += target
