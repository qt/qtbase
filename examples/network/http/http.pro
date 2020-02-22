QT += network widgets
android: qtHaveModule(androidextras) {
    QT += androidextras
    DEFINES += REQUEST_PERMISSIONS_ON_ANDROID
}

HEADERS += httpwindow.h
SOURCES += httpwindow.cpp \
           main.cpp
FORMS += authenticationdialog.ui

# install
target.path = $$[QT_INSTALL_EXAMPLES]/network/http
INSTALLS += target
