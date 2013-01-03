RESOURCES += images.qrc

HEADERS += mainwindow.h view.h chip.h
SOURCES += main.cpp
SOURCES += mainwindow.cpp view.cpp chip.cpp

qtHaveModule(opengl): QT += opengl

build_all:!build_pass {
    CONFIG -= build_all
    CONFIG += release
}

# install
target.path = $$[QT_INSTALL_EXAMPLES]/graphicsview/chip
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.png *.pro *.html *.doc images
sources.path = $$[QT_INSTALL_EXAMPLES]/graphicsview/chip
INSTALLS += target sources

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
