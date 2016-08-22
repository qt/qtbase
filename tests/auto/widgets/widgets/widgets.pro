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
   qfocusframe \
   qfontcombobox \
   qframe \
   qgroupbox \
   qkeysequenceedit \
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

# The following tests depend on private API:
!qtConfig(private_tests): SUBDIRS -= \
           qcombobox \
           qmainwindow \
           qtextedit \
           qtoolbar \

qtConfig(opengl): SUBDIRS += qopenglwidget
