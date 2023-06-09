TEMPLATE = app

QT += qml widgets

#! [0] #! [qrc]
RESOURCES = application.qrc
#! [0] #! [qrc]

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
