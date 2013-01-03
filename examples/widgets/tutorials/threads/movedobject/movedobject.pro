QT += widgets
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app
SOURCES += main.cpp \
    workerobject.cpp \
    thread.cpp

HEADERS += \
    workerobject.h \
    thread.h

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/tutorials/threads/movedobject
INSTALLS += target
