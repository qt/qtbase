TEMPLATE = app

QT += qml widgets

#! [0]
RESOURCES = application.qrc
#! [0]

#! [1]
resources.files = \
    images/copy.png \
    images/cut.png \
    images/new.png \
    images/open.png \
    images/paste.png \
    images/save.png
resources.prefix = /

RESOURCES = resources
#! [1]

#! [2]
CONFIG += resources_big
#! [2]

SOURCES += main.cpp
