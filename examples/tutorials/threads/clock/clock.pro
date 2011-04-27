CONFIG += console
TEMPLATE = app
SOURCES += main.cpp \
    clockthread.cpp
HEADERS += clockthread.h

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tutorials/threads/clock
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS clock.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tutorials/threads/clock
INSTALLS += target sources

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)

