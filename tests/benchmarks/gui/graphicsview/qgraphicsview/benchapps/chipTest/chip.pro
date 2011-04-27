RESOURCES += images.qrc

HEADERS += mainwindow.h view.h chip.h
SOURCES += main.cpp
SOURCES += mainwindow.cpp view.cpp chip.cpp

contains(QT_CONFIG, opengl):QT += opengl

build_all:!build_pass {
    CONFIG -= build_all
    CONFIG += release
}

# install
target.path = $$[QT_INSTALL_DEMOS]/chip
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.png *.pro *.html *.doc images
sources.path = $$[QT_INSTALL_DEMOS]/chip
INSTALLS += target sources

