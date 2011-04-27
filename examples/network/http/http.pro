HEADERS += httpwindow.h
SOURCES += httpwindow.cpp \
           main.cpp
FORMS += authenticationdialog.ui
QT += network

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/network/http
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS http.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/network/http
INSTALLS += target sources

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
