TEMPLATE = app
QT += widgets
requires(qtConfig(treeview))

HEADERS += model.h
SOURCES += model.cpp main.cpp
RESOURCES += interview.qrc

build_all:!build_pass {
    CONFIG -= build_all
    CONFIG += release
}

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/itemviews/interview
INSTALLS += target
