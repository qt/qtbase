QT       += core gui
TARGET = widgetgrab
TEMPLATE = app

SOURCES += main.cpp

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += widgets
}
