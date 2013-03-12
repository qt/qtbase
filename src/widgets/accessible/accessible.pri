# Qt accessibility module

contains(QT_CONFIG, accessibility) {
    HEADERS += accessible/qaccessiblewidget_p.h
    SOURCES += accessible/qaccessiblewidget.cpp
}
