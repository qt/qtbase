HEADERS   = database.h \
            dialog.h \
            mainwindow.h
RESOURCES = masterdetail.qrc
SOURCES   = dialog.cpp \
            main.cpp \
            mainwindow.cpp

QT += sql widgets
QT += xml widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/sql/masterdetail
sources.files = $$SOURCES *.h $$RESOURCES $$FORMS masterdetail.pro *.xml images
sources.path = $$[QT_INSTALL_EXAMPLES]/sql/masterdetail
INSTALLS += target sources
