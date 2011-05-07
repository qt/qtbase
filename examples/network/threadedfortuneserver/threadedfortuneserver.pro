QT += widgets

HEADERS       = dialog.h \
                fortuneserver.h \
                fortunethread.h
SOURCES       = dialog.cpp \
                fortuneserver.cpp \
                fortunethread.cpp \
                main.cpp
QT           += network

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/network/threadedfortuneserver
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS threadedfortuneserver.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/network/threadedfortuneserver
INSTALLS += target sources

symbian: CONFIG += qt_example
