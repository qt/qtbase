TEMPLATE=subdirs
SUBDIRS=\
   collections \
   exceptionsafety \
   exceptionsafety_objects \
   gestures \
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
   qobjectrace \
   qsharedpointer_and_qwidget \
   windowsmobile \

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
