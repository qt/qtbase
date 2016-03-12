TEMPLATE=subdirs
SUBDIRS=\
   # atwrapper \ # QTBUG-19452
   compiler \
   gestures \
   lancelot \
   languagechange \
   macgui \
   macnativeevents \
   macplist \
   modeltest \
   networkselftest \
   qaccessibility \
   # qaccessibilitylinux \ # QTBUG-44434
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
   toolsupport \

!qtHaveModule(gui): SUBDIRS -= \
   qcomplextext \
   qprocess_and_guieventloop \

!qtHaveModule(widgets): SUBDIRS -= \
   gestures \
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
   lancelot \
   networkselftest \
   qnetworkaccessmanager_and_qprogressdialog \
   qobjectperformance

cross_compile: SUBDIRS -= \
   atwrapper \
   compiler

wince*|!contains(QT_CONFIG, accessibility): SUBDIRS -= qaccessibility

!contains(QT_CONFIG, accessibility-atspi-bridge): SUBDIRS -= qaccessibilitylinux

!mac: SUBDIRS -= \
           macgui \
           macnativeevents \
           macplist \
           qaccessibilitymac

!embedded|wince: SUBDIRS -= \
           qdirectpainter

winrt: SUBDIRS -= \
   qprocess_and_guieventloop

android: SUBDIRS += \
    android
