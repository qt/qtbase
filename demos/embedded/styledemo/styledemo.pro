TEMPLATE = app

# Input
HEADERS += stylewidget.h
FORMS += stylewidget.ui
SOURCES += main.cpp stylewidget.cpp
RESOURCES += styledemo.qrc

target.path = $$[QT_INSTALL_DEMOS]/qtbase/embedded/styledemo
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro *.html
sources.path = $$[QT_INSTALL_DEMOS]/qtbase/embedded/styledemo
INSTALLS += target sources

symbian {
    TARGET.UID3 = 0xA000A63F
    CONFIG += qt_demo
}
QT += widgets widgets
