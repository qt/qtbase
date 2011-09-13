TEMPLATE = app

# Input
HEADERS += stylewidget.h
FORMS += stylewidget.ui
SOURCES += main.cpp stylewidget.cpp
RESOURCES += styleexample.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/embedded/styleexample
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro *.html
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/embedded/styleexample
INSTALLS += target sources

symbian {
    TARGET.UID3 = 0xA000A63F
    CONFIG += qt_example
}
QT += widgets widgets
