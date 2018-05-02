# Qt widgets module

HEADERS += \
        widgets/qframe.h \
        widgets/qframe_p.h \
        widgets/qabstractscrollarea.h \
        widgets/qabstractscrollarea_p.h \
        widgets/qfocusframe.h \
        widgets/qwidgetanimator_p.h

SOURCES += \
        widgets/qframe.cpp \
        widgets/qabstractscrollarea.cpp \
        widgets/qfocusframe.cpp \
        widgets/qwidgetanimator.cpp

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

qtConfig(buttongroup) {
    HEADERS += \
        widgets/qbuttongroup.h \
        widgets/qbuttongroup_p.h

    SOURCES += widgets/qbuttongroup.cpp
}

qtConfig(calendarwidget) {
    HEADERS += widgets/qcalendarwidget.h
    SOURCES += widgets/qcalendarwidget.cpp
}

qtConfig(checkbox) {
    HEADERS += \
        widgets/qcheckbox.h

    SOURCES += \
        widgets/qcheckbox.cpp
}

qtConfig(combobox) {
    HEADERS += \
        widgets/qcombobox.h \
        widgets/qcombobox_p.h

    SOURCES += widgets/qcombobox.cpp
}

qtConfig(commandlinkbutton) {
    HEADERS += \
        widgets/qcommandlinkbutton.h

    SOURCES += \
        widgets/qcommandlinkbutton.cpp
}

qtConfig(datetimeedit) {
    HEADERS += \
         widgets/qdatetimeedit.h \
         widgets/qdatetimeedit_p.h

    SOURCES += \
         widgets/qdatetimeedit.cpp
}

qtConfig(dial) {
    HEADERS += widgets/qdial.h
    SOURCES += widgets/qdial.cpp
}

qtConfig(dockwidget) {
    HEADERS += \
        widgets/qdockwidget.h \
        widgets/qdockwidget_p.h \
        widgets/qdockarealayout_p.h

    SOURCES += \
        widgets/qdockwidget.cpp \
        widgets/qdockarealayout.cpp
}

qtConfig(effects) {
    HEADERS += widgets/qeffects_p.h
    SOURCES += widgets/qeffects.cpp
}

qtConfig(fontcombobox) {
    HEADERS += widgets/qfontcombobox.h
    SOURCES += widgets/qfontcombobox.cpp
}

qtConfig(groupbox) {
    HEADERS += widgets/qgroupbox.h
    SOURCES += widgets/qgroupbox.cpp
}

qtConfig(keysequenceedit) {
    HEADERS += \
        widgets/qkeysequenceedit.h \
        widgets/qkeysequenceedit_p.h

    SOURCES += widgets/qkeysequenceedit.cpp
}

qtConfig(label) {
    HEADERS += \
        widgets/qlabel.h \
        widgets/qlabel_p.h

    SOURCES += \
        widgets/qlabel.cpp
}

qtConfig(lcdnumber) {
    HEADERS += \
        widgets/qlcdnumber.h

    SOURCES += \
        widgets/qlcdnumber.cpp
}

qtConfig(lineedit) {
    HEADERS += \
        widgets/qlineedit.h \
        widgets/qlineedit_p.h \
        widgets/qwidgetlinecontrol_p.h

    SOURCES += \
        widgets/qlineedit_p.cpp \
        widgets/qlineedit.cpp \
        widgets/qwidgetlinecontrol.cpp
}

qtConfig(mainwindow) {
    HEADERS += \
        widgets/qmainwindow.h \
        widgets/qmainwindowlayout_p.h

    SOURCES += \
        widgets/qmainwindow.cpp \
        widgets/qmainwindowlayout.cpp
}

qtConfig(mdiarea) {
    HEADERS += \
        widgets/qmdiarea.h \
        widgets/qmdiarea_p.h \
        widgets/qmdisubwindow.h \
        widgets/qmdisubwindow_p.h

    SOURCES += \
        widgets/qmdiarea.cpp \
        widgets/qmdisubwindow.cpp
}

qtConfig(menu) {
    HEADERS += \
        widgets/qmenu.h \
        widgets/qmenu_p.h

    SOURCES += widgets/qmenu.cpp
}

qtConfig(menubar) {
    HEADERS += \
        widgets/qmenubar.h \
        widgets/qmenubar_p.h

    SOURCES += widgets/qmenubar.cpp
}

qtConfig(progressbar) {
    HEADERS += widgets/qprogressbar.h
    SOURCES += widgets/qprogressbar.cpp
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

qtConfig(resizehandler) {
    HEADERS += widgets/qwidgetresizehandler_p.h
    SOURCES += widgets/qwidgetresizehandler.cpp
}

qtConfig(dialogbuttonbox) {
    HEADERS += \
        widgets/qdialogbuttonbox.h

    SOURCES += \
        widgets/qdialogbuttonbox.cpp
}

qtConfig(rubberband) {
    HEADERS += widgets/qrubberband.h
    SOURCES += widgets/qrubberband.cpp
}

qtConfig(scrollarea) {
    HEADERS += \
        widgets/qscrollarea.h \
        widgets/qscrollarea_p.h

    SOURCES += widgets/qscrollarea.cpp
}

qtConfig(scrollbar) {
    HEADERS += \
        widgets/qscrollbar.h \
        widgets/qscrollbar_p.h

    SOURCES += widgets/qscrollbar.cpp
}

qtConfig(sizegrip) {
    HEADERS += widgets/qsizegrip.h
    SOURCES += widgets/qsizegrip.cpp
}

qtConfig(slider) {
    HEADERS += widgets/qslider.h
    SOURCES += widgets/qslider.cpp
}

qtConfig(spinbox) {
    HEADERS += \
        widgets/qabstractspinbox.h \
        widgets/qabstractspinbox_p.h \
        widgets/qspinbox.h

    SOURCES += \
        widgets/qabstractspinbox.cpp \
        widgets/qspinbox.cpp
}

qtConfig(splashscreen) {
    HEADERS += \
        widgets/qsplashscreen.h

    SOURCES += \
        widgets/qsplashscreen.cpp
}

qtConfig(splitter) {
    HEADERS += \
        widgets/qsplitter.h \
        widgets/qsplitter_p.h

    SOURCES += widgets/qsplitter.cpp
}

qtConfig(stackedwidget) {
    HEADERS += widgets/qstackedwidget.h
    SOURCES += widgets/qstackedwidget.cpp
}

qtConfig(statusbar) {
    HEADERS += widgets/qstatusbar.h
    SOURCES += widgets/qstatusbar.cpp
}

qtConfig(tabbar) {
    HEADERS += \
        widgets/qtabbar.h \
        widgets/qtabbar_p.h

    SOURCES += widgets/qtabbar.cpp
}

qtConfig(textedit) {
    HEADERS += \
        widgets/qplaintextedit.h \
        widgets/qplaintextedit_p.h \
        widgets/qtextedit.h \
        widgets/qtextedit_p.h

    SOURCES += \
        widgets/qplaintextedit.cpp \
        widgets/qtextedit.cpp
}

qtConfig(textbrowser) {
    HEADERS += widgets/qtextbrowser.h
    SOURCES += widgets/qtextbrowser.cpp
}

qtConfig(tabwidget) {
    HEADERS += widgets/qtabwidget.h
    SOURCES += widgets/qtabwidget.cpp
}

qtConfig(toolbar) {
    HEADERS += \
        widgets/qtoolbar.h \
        widgets/qtoolbar_p.h \
        widgets/qtoolbararealayout_p.h \
        widgets/qtoolbarlayout_p.h \
        widgets/qtoolbarseparator_p.h

    SOURCES += \
        widgets/qtoolbar.cpp \
        widgets/qtoolbarlayout.cpp \
        widgets/qtoolbararealayout.cpp \
        widgets/qtoolbarseparator.cpp
}

qtConfig(toolbox) {
    HEADERS += widgets/qtoolbox.h
    SOURCES += widgets/qtoolbox.cpp
}

qtConfig(toolbutton) {
    HEADERS += \
        widgets/qtoolbutton.h \
        widgets/qtoolbarextension_p.h

    SOURCES += \
        widgets/qtoolbutton.cpp \
        widgets/qtoolbarextension.cpp
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
        widgets/qmacnativewidget_mac.mm \
        widgets/qmaccocoaviewcontainer_mac.mm

    qtConfig(menu)|qtConfig(menubar) {
        SOURCES += widgets/qmenu_mac.mm
    }
}
