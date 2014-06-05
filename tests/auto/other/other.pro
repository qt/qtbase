TEMPLATE=subdirs
SUBDIRS=\
   # atwrapper \ # QTBUG-19452
   baselineexample \
   collections \
   compiler \
   d3dcompiler \
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
   qaccessibilitylinux \
   qaccessibilitymac \
   qcomplextext \
   qfocusevent \
   qnetworkaccessmanager_and_qprogressdialog \
   qobjectperformance \
   qobjectrace \
   qsharedpointer_and_qwidget \
   qprocess_and_guieventloop \
   qtokenautomaton \
   windowsmobile \

!qtHaveModule(widgets): SUBDIRS -= \
   baselineexample \
   gestures \
   headersclean \
   lancelot \
   languagechange \
   modeltest \
   qaccessibility \
   qfocusevent \
   qnetworkaccessmanager_and_qprogressdialog \
   qsharedpointer_and_qwidget \
   windowsmobile \
   qaccessibility \
   qaccessibilitylinux \
   qaccessibilitymac \

!qtHaveModule(network): SUBDIRS -= \
   baselineexample \
   headersclean \
   lancelot \
   networkselftest \
   qnetworkaccessmanager_and_qprogressdialog \
   qobjectperformance

testcocoon: SUBDIRS -= headersclean

cross_compile: SUBDIRS -= \
   atwrapper \
   compiler

wince*|!contains(QT_CONFIG, accessibility): SUBDIRS -= qaccessibility

!angle_d3d11: SUBDIRS -= d3dcompiler

!contains(QT_CONFIG, accessibility-atspi-bridge): SUBDIRS -= qaccessibilitylinux

!mac: SUBDIRS -= \
           macgui \
           macnativeevents \
           macplist \
           qaccessibilitymac

!embedded|wince*: SUBDIRS -= \
           qdirectpainter

winrt: SUBDIRS -= \
   qprocess_and_guieventloop
