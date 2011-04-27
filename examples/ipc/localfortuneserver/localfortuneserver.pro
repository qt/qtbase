HEADERS       = server.h
SOURCES       = server.cpp \
                main.cpp
QT           += network

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/ipc/localfortuneserver
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS localfortuneserver.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/ipc/localfortuneserver
INSTALLS += target sources

symbian: CONFIG += qt_example
maemo5: CONFIG += qt_example

simulator: warning(This example might not fully work on Simulator platform)
