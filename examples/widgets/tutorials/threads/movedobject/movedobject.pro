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
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/tutorials/threads/movedobject
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS movedobject.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/tutorials/threads/movedobject
INSTALLS += target sources

QT += widgets
