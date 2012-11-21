TEMPLATE = app

CONFIG += qt warn_on
HEADERS += model.h
SOURCES += model.cpp main.cpp
RESOURCES += interview.qrc

build_all:!build_pass {
    CONFIG -= build_all
    CONFIG += release
}

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/itemviews/interview
sources.files = $$SOURCES $$HEADERS $$RESOURCES README *.pro images
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/itemviews/interview
INSTALLS += target sources

QT += widgets
