# Qt widgets module

HEADERS += \
        widgets/qbuttongroup.h \
        widgets/qbuttongroup_p.h \
        widgets/qabstractspinbox.h \
        widgets/qabstractspinbox_p.h \
        widgets/qcalendarwidget.h \
        widgets/qcombobox.h \
        widgets/qcombobox_p.h \
        widgets/qdatetimeedit.h \
        widgets/qdatetimeedit_p.h \
        widgets/qdial.h \
        widgets/qdockwidget.h \
        widgets/qdockwidget_p.h \
        widgets/qdockarealayout_p.h \
        widgets/qfontcombobox.h \
        widgets/qframe.h \
        widgets/qframe_p.h \
        widgets/qgroupbox.h \
        widgets/qkeysequenceedit.h \
        widgets/qkeysequenceedit_p.h \
        widgets/qlcdnumber.h \
        widgets/qlineedit.h \
        widgets/qlineedit_p.h \
        widgets/qmainwindow.h \
        widgets/qmainwindowlayout_p.h \
        widgets/qmdiarea.h \
        widgets/qmdiarea_p.h \
        widgets/qmdisubwindow.h \
        widgets/qmdisubwindow_p.h \
        widgets/qmenu.h \
        widgets/qmenu_p.h \
        widgets/qmenubar.h \
        widgets/qmenubar_p.h \
        widgets/qprogressbar.h \
        widgets/qrubberband.h \
        widgets/qscrollbar.h \
        widgets/qscrollbar_p.h \
        widgets/qscrollarea_p.h \
        widgets/qsizegrip.h \
        widgets/qslider.h \
        widgets/qspinbox.h \
        widgets/qsplashscreen.h \
        widgets/qsplitter.h \
        widgets/qsplitter_p.h \
        widgets/qstackedwidget.h \
        widgets/qstatusbar.h \
        widgets/qtabbar.h \
        widgets/qtabbar_p.h \
        widgets/qtabwidget.h \
        widgets/qtextedit.h \
        widgets/qtextedit_p.h \
        widgets/qtextbrowser.h \
        widgets/qtoolbar.h \
        widgets/qtoolbar_p.h \
        widgets/qtoolbarlayout_p.h \
        widgets/qtoolbarextension_p.h \
        widgets/qtoolbarseparator_p.h \
        widgets/qtoolbox.h \
        widgets/qtoolbutton.h \
        widgets/qabstractscrollarea.h \
        widgets/qabstractscrollarea_p.h \
        widgets/qwidgetresizehandler_p.h \
        widgets/qfocusframe.h \
        widgets/qscrollarea.h \
        widgets/qwidgetanimator_p.h \
        widgets/qwidgetlinecontrol_p.h \
        widgets/qtoolbararealayout_p.h \
        widgets/qplaintextedit.h \
        widgets/qplaintextedit_p.h

SOURCES += \
        widgets/qbuttongroup.cpp \
        widgets/qabstractspinbox.cpp \
        widgets/qcalendarwidget.cpp \
        widgets/qcombobox.cpp \
        widgets/qdatetimeedit.cpp \
        widgets/qdial.cpp \
        widgets/qdockwidget.cpp \
        widgets/qdockarealayout.cpp \
        widgets/qeffects.cpp \
        widgets/qfontcombobox.cpp \
        widgets/qframe.cpp \
        widgets/qgroupbox.cpp \
        widgets/qkeysequenceedit.cpp \
        widgets/qlcdnumber.cpp \
        widgets/qlineedit_p.cpp \
        widgets/qlineedit.cpp \
        widgets/qmainwindow.cpp \
        widgets/qmainwindowlayout.cpp \
        widgets/qmdiarea.cpp \
        widgets/qmdisubwindow.cpp \
        widgets/qmenu.cpp \
        widgets/qmenubar.cpp \
        widgets/qprogressbar.cpp \
        widgets/qrubberband.cpp \
        widgets/qscrollbar.cpp \
        widgets/qsizegrip.cpp \
        widgets/qslider.cpp \
        widgets/qspinbox.cpp \
        widgets/qsplashscreen.cpp \
        widgets/qsplitter.cpp \
        widgets/qstackedwidget.cpp \
        widgets/qstatusbar.cpp \
        widgets/qtabbar.cpp \
        widgets/qtabwidget.cpp \
        widgets/qtextedit.cpp \
        widgets/qtextbrowser.cpp \
        widgets/qtoolbar.cpp \
        widgets/qtoolbarlayout.cpp \
        widgets/qtoolbarextension.cpp \
        widgets/qtoolbarseparator.cpp \
        widgets/qtoolbox.cpp \
        widgets/qtoolbutton.cpp \
        widgets/qabstractscrollarea.cpp \
        widgets/qwidgetresizehandler.cpp \
        widgets/qfocusframe.cpp \
        widgets/qscrollarea.cpp \
        widgets/qwidgetanimator.cpp \
        widgets/qwidgetlinecontrol.cpp \
        widgets/qtoolbararealayout.cpp \
        widgets/qplaintextedit.cpp

qtConfig(abstractbutton) {
    HEADERS += \
        widgets/qabstractbutton.h \
        widgets/qabstractbutton_p.h

    SOURCES += \
        widgets/qabstractbutton.cpp
}

qtConfig(abstractslider) {
    HEADERS += \
        widgets/qabstractslider.h \
        widgets/qabstractslider_p.h

    SOURCES += \
        widgets/qabstractslider.cpp
}

qtConfig(checkbox) {
    HEADERS += \
        widgets/qcheckbox.h

    SOURCES += \
        widgets/qcheckbox.cpp
}

qtConfig(commandlinkbutton) {
    HEADERS += \
        widgets/qcommandlinkbutton.h

    SOURCES += \
        widgets/qcommandlinkbutton.cpp
}

qtConfig(label) {
    HEADERS += \
        widgets/qlabel.h \
        widgets/qlabel_p.h

    SOURCES += \
        widgets/qlabel.cpp
}


qtConfig(pushbutton) {
    HEADERS += \
        widgets/qpushbutton.h \
        widgets/qpushbutton_p.h

    SOURCES += \
        widgets/qpushbutton.cpp
}

qtConfig(radiobutton) {
    HEADERS += \
        widgets/qradiobutton.h

    SOURCES += \
        widgets/qradiobutton.cpp
}

qtConfig(dialogbuttonbox) {
    HEADERS += \
        widgets/qdialogbuttonbox.h

    SOURCES += \
        widgets/qdialogbuttonbox.cpp
}

qtConfig(widgettextcontrol) {
    HEADERS += \
        widgets/qwidgettextcontrol_p.h \
        widgets/qwidgettextcontrol_p_p.h

    SOURCES += \
        widgets/qwidgettextcontrol.cpp
}

macx {
    HEADERS += \
        widgets/qmacnativewidget_mac.h \
        widgets/qmaccocoaviewcontainer_mac.h

    OBJECTIVE_SOURCES += \
        widgets/qmenu_mac.mm \
        widgets/qmacnativewidget_mac.mm \
        widgets/qmaccocoaviewcontainer_mac.mm
}
