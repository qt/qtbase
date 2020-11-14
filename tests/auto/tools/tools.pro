TEMPLATE=subdirs
# QTBUG-88538
!android:!ios: SUBDIRS = \
   qmakelib \
   qmake \
   uic \
   moc \
   rcc \

qtHaveModule(dbus): SUBDIRS += qdbuscpp2xml qdbusxml2cpp
!qtHaveModule(widgets): SUBDIRS -= uic
