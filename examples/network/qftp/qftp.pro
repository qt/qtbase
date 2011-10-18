QT += widgets

HEADERS       = ftpwindow.h
SOURCES       = ftpwindow.cpp \
                main.cpp
RESOURCES    += ftp.qrc
QT           += network

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/network/qftp
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro images
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/network/qftp
INSTALLS += target sources
