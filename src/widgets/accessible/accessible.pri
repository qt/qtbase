# Qt accessibility module

contains(QT_CONFIG, accessibility) {
    HEADERS += \
        accessible/qaccessiblewidget.h \
        accessible/qaccessiblewidgetfactory_p.h \
        accessible/complexwidgets_p.h \
        accessible/itemviews_p.h \
        accessible/qaccessiblemenu_p.h \
        accessible/qaccessiblewidgets_p.h \
        accessible/rangecontrols_p.h \
        accessible/simplewidgets_p.h

    SOURCES += \
        accessible/qaccessiblewidget.cpp \
        accessible/qaccessiblewidgetfactory.cpp \
        accessible/complexwidgets.cpp \
        accessible/itemviews.cpp \
        accessible/qaccessiblemenu.cpp \
        accessible/qaccessiblewidgets.cpp \
        accessible/rangecontrols.cpp \
        accessible/simplewidgets.cpp
}
