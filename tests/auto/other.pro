# These tests don't nicely fit into one of the other .pro files.
# They are testing too many Qt modules at the same time.

TEMPLATE=subdirs
SUBDIRS=\
           other \
#           baselineexample \ Just an example demonstrating qbaselinetest usage
           exceptionsafety_objects \
           lancelot \
           macgui \
           macnativeevents \
           macplist \
           qaccessibility \
           qcombobox \
           qcopchannel \
           qdirectpainter \
           qfocusevent \
           qlayout \
           qmdiarea \
           qmenu \
           qmenubar \
           qmultiscreen \
           qsplitter \
           qtabwidget \
           qtextbrowser \
           qtextedit \
           qtoolbutton \
           qwidget \
           qworkspace \
           windowsmobile \
           qmetaobjectbuilder

wince*|!contains(QT_CONFIG, accessibility):SUBDIRS -= qaccessibility

!mac|qpa: SUBDIRS -= \
           macgui \
           macnativeevents \
           macplist \

!embedded|wince*: SUBDIRS -= \
           qcopchannel \
           qdirectpainter \
           qmultiscreen \

!linux*-g++*:SUBDIRS -= exceptionsafety_objects

# Following tests depends on private API
!contains(QT_CONFIG, private_tests): SUBDIRS -= \
           qcombobox \
           qtextedit \
