HEADERS   = database.h \
            dialog.h \
            mainwindow.h
RESOURCES = masterdetail.qrc
SOURCES   = dialog.cpp \
            main.cpp \
            mainwindow.cpp

QT += sql
QT += xml

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/sql/masterdetail
sources.files = $$SOURCES *.h $$RESOURCES $$FORMS masterdetail.pro *.xml images
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/sql/masterdetail
INSTALLS += target sources

symbian {
    TARGET.UID3 = 0xA000D7CF
    CONFIG += qt_example
}
