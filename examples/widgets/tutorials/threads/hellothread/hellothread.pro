QT -= gui

CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app
SOURCES += main.cpp \
    hellothread.cpp 
HEADERS += hellothread.h 

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/tutorials/threads/hellothread
INSTALLS += target


