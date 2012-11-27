HEADERS   = database.h \
            dialog.h \
            mainwindow.h
RESOURCES = masterdetail.qrc
SOURCES   = dialog.cpp \
            main.cpp \
            mainwindow.cpp

QT += sql widgets
QT += xml widgets

EXAMPLE_FILES = albumdetails.xml

# install
target.path = $$[QT_INSTALL_EXAMPLES]/sql/masterdetail
INSTALLS += target
