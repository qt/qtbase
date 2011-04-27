HEADERS       = sender.h
SOURCES       = sender.cpp \
                main.cpp
QT           += network

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/network/multicastsender
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS multicastsender.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/network/multicastsender
INSTALLS += target sources

symbian: CONFIG += qt_example
