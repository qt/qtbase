TEMPLATE=subdirs
QT_FOR_CONFIG += gui-private

SUBDIRS=\
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

!qtConfig(accessibility): SUBDIRS -= qaccessibility

!qtConfig(accessibility-atspi-bridge): SUBDIRS -= qaccessibilitylinux

!qtConfig(process): SUBDIRS -= qprocess_and_guieventloop

!mac: SUBDIRS -= \
           macgui \
           macnativeevents \
           macplist \
           qaccessibilitymac

!embedded: SUBDIRS -= \
           qdirectpainter

android: SUBDIRS += \
    android
