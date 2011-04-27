COMMON_FOLDER = $$PWD/../common
include(../arthurtester.pri)
CONFIG+=console moc
TEMPLATE = app
INCLUDEPATH += .

# Input
HEADERS += widgets.h interactivewidget.h
SOURCES += interactivewidget.cpp main.cpp
RESOURCES += icons.qrc

contains(QT_CONFIG, opengl)|contains(QT_CONFIG, opengles1)|contains(QT_CONFIG, opengles2):QT += opengl
contains(QT_CONFIG, qt3support):QT += qt3support

symbian*: {
    testData.files = $$QT_BUILD_TREE/tests/arthur/data/qps
    testData.path = .
    DEPLOYMENT += testData
}

QT += xml svg 


