# These tests don't nicely fit into one of the other .pro files.
# They are testing too many Qt modules at the same time.

TEMPLATE=subdirs
SUBDIRS=\
#           baselineexample \ Just an example demonstrating qbaselinetest usage
           lancelot \
           qalgorithms \
           qcombobox \
           qcssparser \
           qdatastream \
           qdir \
           qfocusevent \
           qimage \
           qiodevice \
           qitemmodel \
           qlayout \
           qmdiarea \
           qmenu \
           qmenubar \
           qmouseevent \
           qpainter \
           qpixmap \
           qprinter \
           qsettings \
           qsplitter \
           qtabwidget \
           qtextbrowser \
           qtextdocument \
           qtextedit \
           qtoolbutton \
           qvariant \
           qwidget \
           qworkspace \
           windowsmobile \
           nativeimagehandleprovider

contains(QT_CONFIG, accessibility):SUBDIRS += qaccessibility

contains(QT_CONFIG, OdfWriter):SUBDIRS += qzip qtextodfwriter
mac: {
    SUBDIRS += macgui \
               macnativeevents \
               macplist \
               qaccessibility_mac
}

embedded:!wince* {
    SUBDIRS += qcopchannel \
               qdirectpainter \
               qmultiscreen
}

symbian {
    SUBDIRS += qsoftkeymanager \
               qs60mainapplication
}

!win32-msvc*:!wince*:SUBDIRS += exceptionsafety_objects

# Following tests depends on private API
!contains(QT_CONFIG, private_tests): SUBDIRS -= \
           qcombobox \
           qcssparser \
           qtextedit \

