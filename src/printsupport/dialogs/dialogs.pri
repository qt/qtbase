# Qt dialogs module

INCLUDEPATH += $$PWD

qtConfig(printdialog) {
    HEADERS += \
        dialogs/qabstractprintdialog.h \
        dialogs/qabstractprintdialog_p.h \
        dialogs/qpagesetupdialog_p.h \
        dialogs/qpagesetupdialog.h \
        dialogs/qprintdialog.h

    macos {
        OBJECTIVE_SOURCES += dialogs/qpagesetupdialog_mac.mm \
                             dialogs/qprintdialog_mac.mm
        LIBS_PRIVATE += -framework AppKit
    }

    win32 {
        SOURCES += dialogs/qpagesetupdialog_win.cpp \
                   dialogs/qprintdialog_win.cpp
    }

    unix:!darwin {
        INCLUDEPATH += $$QT_SOURCE_TREE/src/plugins/printsupport/cups
        HEADERS += dialogs/qpagesetupdialog_unix_p.h
        SOURCES += dialogs/qprintdialog_unix.cpp \
                   dialogs/qpagesetupdialog_unix.cpp
        FORMS += dialogs/qprintsettingsoutput.ui \
        dialogs/qprintwidget.ui \
        dialogs/qprintpropertieswidget.ui
    }

    SOURCES += \
        dialogs/qabstractprintdialog.cpp \
        dialogs/qpagesetupdialog.cpp

    FORMS += dialogs/qpagesetupwidget.ui
    RESOURCES += dialogs/qprintdialog.qrc
}

qtConfig(printpreviewdialog) {
    HEADERS += dialogs/qprintpreviewdialog.h
    SOURCES += dialogs/qprintpreviewdialog.cpp
}

