# The tests in this .pro file _MUST_ use QtCore, QtNetwork and QtGui only
# (i.e. QT=core gui network).
# The test system is allowed to run these tests before the rest of Qt has
# been compiled.
#
TEMPLATE=subdirs
SUBDIRS=\
    gui \
    gestures \
    languagechange \
    modeltest \
    qabstractbutton \
    qabstractitemview \
    qabstractprintdialog \
    qabstractproxymodel \
    qabstractscrollarea \
    qabstractslider \
    qabstractspinbox \
    qaction \
    qactiongroup \
    qapplication \
    qboxlayout \
    qbuttongroup \
    qcalendarwidget \
    qcheckbox \
    qcolordialog \
    qcolumnview \
    qcommandlinkbutton \
    qcompleter \
    qcomplextext \
    qdatawidgetmapper \
    qdatetimeedit \
    qdesktopwidget \
    qdial \
    qdialog \
    qdialogbuttonbox \
    qdirmodel \
    qdockwidget \
    qdoublespinbox \
    qdoublevalidator \
    qerrormessage \
    qfiledialog \
    qfiledialog2 \
    qfileiconprovider \
    qfilesystemmodel \
    qfocusframe \
    qfontcombobox \
    qfontdialog \
    qformlayout \
    qgraphicsanchorlayout \
    qgraphicsanchorlayout1 \
    qgraphicseffect \
    qgraphicseffectsource \
    qgraphicsgridlayout \
    qgraphicsitem \
    qgraphicsitemanimation \
    qgraphicslayout \
    qgraphicslayoutitem \
    qgraphicslinearlayout \
    qgraphicsobject \
    qgraphicspixmapitem \
    qgraphicspolygonitem \
    qgraphicsproxywidget \
    qgraphicsscene \
    qgraphicssceneindex \
    qgraphicstransform \
    qgraphicsview \
    qgraphicswidget \
    qgridlayout \
    qgroupbox \
    qheaderview \
    qidentityproxymodel \
    qinputcontext \
    qinputdialog \
    qintvalidator \
    qitemdelegate \
    qitemeditorfactory \
    qitemselectionmodel \
    qitemview \
    qlabel \
    qlcdnumber \
    qlineedit \
    qlistview \
    qlistwidget \
    qmacstyle \
    qmainwindow \
    qmdisubwindow \
    qmessagebox \
    qnetworkaccessmanager_and_qprogressdialog \
    qopengl \
    qplaintextedit \
    qprogressbar \
    qprogressdialog \
    qpushbutton \
    qradiobutton \
    qregexpvalidator \
    qscrollarea \
    qscrollbar \
    qscroller \
    qsharedpointer_and_qwidget \
    qsidebar \
    qsizegrip \
    qslider \
    qsortfilterproxymodel \
    qsound \
    qspinbox \
    qstackedlayout \
    qstackedwidget \
    qstandarditem \
    qstandarditemmodel \
    qstatusbar \
    qstringlistmodel \
    qstyle \
    qstyleoption \
    qstylesheetstyle \
    qsystemtrayicon \
    qtabbar \
    qtableview \
    qtablewidget \
    qtoolbar \
    qtoolbox \
    qtooltip \
    qtransformedscreen \
    qtreeview \
    qtreewidget \
    qtreewidgetitemiterator \
    qundogroup \
    qundostack \
    qwidget_window \
    qwidgetaction \
    qwindowsurface \
    qwizard \
    qwsembedwidget \
    qwsinputmethod \
    qwswindowsystem \
    qx11info \

# This test cannot be run on Mac OS
mac*:SUBDIRS -= qwindowsurface

# This test takes too long to run on IRIX, so skip it on that platform
irix-*:SUBDIRS -= qitemview

# These tests are only valid for QWS
!embedded|wince*:SUBDIRS -= \
    qtransformedscreen \
    qwsembedwidget \
    qwsinputmethod \
    qwswindowsystem \

win32:SUBDIRS -= qtextpiecetable

!contains(QT_CONFIG, private_tests): SUBDIRS -= \
           qcolumnview \
           qgraphicsanchorlayout \
           qgraphicsanchorlayout1 \
           qgraphicsitem \
           qgraphicsscene \
           qgraphicssceneindex \
           qlistwidget \
           qmainwindow \
           qpathclipper \
           qpixmapcache \
           qsidebar \
           qstylesheetstyle \
           qtextlayout \
           qtextpiecetable \
           qtipc \
           qtoolbar \

