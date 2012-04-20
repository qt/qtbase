TEMPLATE=subdirs
SUBDIRS=\
   # atwrapper \ # QTBUG-19452
   baselineexample \
   collections \
   compiler \
   exceptionsafety \
   # exceptionsafety_objects \    # QObjectPrivate is not safe
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
   qfocusevent \
   qnetworkaccessmanager_and_qprogressdialog \
   qobjectperformance \
   qobjectrace \
   qsharedpointer_and_qwidget \
   qtokenautomaton \
   windowsmobile \

testcocoon: SUBDIRS -= headersclean

cross_compile: SUBDIRS -= \
   atwrapper \
   compiler

wince*|!contains(QT_CONFIG, accessibility):SUBDIRS -= qaccessibility

!mac: SUBDIRS -= \
           macgui \
           macnativeevents \
           macplist

!embedded|wince*: SUBDIRS -= \
           qdirectpainter

!linux*-g++*:SUBDIRS -= exceptionsafety_objects
