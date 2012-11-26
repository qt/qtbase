CONFIG += console
TEMPLATE = app
SOURCES += main.cpp \
    clockthread.cpp
HEADERS += clockthread.h

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/tutorials/threads/clock
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS clock.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/tutorials/threads/clock
INSTALLS += target sources


QT += widgets
