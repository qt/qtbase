TEMPLATE = app
HEADERS += colorswatch.h mainwindow.h toolbar.h
SOURCES += colorswatch.cpp mainwindow.cpp toolbar.cpp main.cpp
build_all:!build_pass {
    CONFIG -= build_all
    CONFIG += release
}

RESOURCES += mainwindow.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/mainwindows/mainwindow
sources.files = $$SOURCES $$HEADERS $$FORMS $$RESOURCES *.png *.jpg *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/mainwindows/mainwindow
INSTALLS += target sources

QT += widgets
