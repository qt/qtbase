CONFIG += testcase
TARGET = tst_qaccessibility
requires(qtConfig(accessibility))
QT += testlib core-private gui-private widgets-private testlib-private
SOURCES += tst_qaccessibility.cpp
HEADERS += accessiblewidgets.h

unix:!darwin:!haiku:!integity: LIBS += -lm

win32 {
    !winrt {
        QT += windowsuiautomation_support-private
    }
    LIBS += -luuid -loleacc -loleaut32 -lole32
}
