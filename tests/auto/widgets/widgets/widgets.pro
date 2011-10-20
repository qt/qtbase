TEMPLATE=subdirs
SUBDIRS=\
   qabstractbutton \
   qabstractscrollarea \
   qabstractslider \
   qabstractspinbox \
   qbuttongroup \
   qcalendarwidget \
   qcheckbox \
   qcombobox \
   qcommandlinkbutton \
   qdatetimeedit \
   qdial \
   qdialogbuttonbox \
   qdockwidget \
   qdoublespinbox \
   qdoublevalidator \
   qfocusframe \
   qfontcombobox \
   qgroupbox \
   qintvalidator \
   qlabel \
   qlcdnumber \
   qlineedit \
   qmainwindow \
   qmdiarea \
   qmdisubwindow \
   qmenu \
   qmenubar \
   qplaintextedit \
   qprogressbar \
   qpushbutton \
   qradiobutton \
   qregexpvalidator \
   qscrollarea \
   qscrollbar \
   qsizegrip \
   qslider \
   qspinbox \
   qsplitter \
   qstackedwidget \
   qstatusbar \
   qtabbar \
   qtabwidget \
   qtextbrowser \
   qtextedit \
   qtoolbar \
   qtoolbox \
   qtoolbutton \
   qworkspace \

# The following tests depend on private API:
!contains(QT_CONFIG, private_tests): SUBDIRS -= \
           qcombobox \
           qmainwindow \
           qtextedit \
           qtoolbar \
