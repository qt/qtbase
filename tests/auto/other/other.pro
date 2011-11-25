TEMPLATE=subdirs
SUBDIRS=\
   # atwrapper \ # QTBUG-19452
   baselineexample \
   collections \
   compiler \
   exceptionsafety \
   exceptionsafety_objects \
   gestures \
   headersclean \
   lancelot \
   languagechange \
   macgui \
   macnativeevents \
   macplist \
   modeltest \
   networkselftest \
   qaccessibility \
   qcomplextext \
   qdirectpainter \
   qfocusevent \
   qmultiscreen \
   qnetworkaccessmanager_and_qprogressdialog \
   qobjectperformance \
   qobjectrace \
   qsharedpointer_and_qwidget \
   qtokenautomaton \
   windowsmobile \

cross_compile: SUBDIRS -= \
   atwrapper \
   compiler \
   headersclean \

wince*|!contains(QT_CONFIG, accessibility):SUBDIRS -= qaccessibility

!mac: SUBDIRS -= \
           macgui \
           macnativeevents \
           macplist

!embedded|wince*: SUBDIRS -= \
           qdirectpainter \
           qmultiscreen \

!linux*-g++*:SUBDIRS -= exceptionsafety_objects

mac: lancelot.CONFIG = no_check_target # QTBUG-22792
