# The tests in this .pro file _MUST_ use QtCore, QtNetwork and QtGui only
# (i.e. QT=core gui network).
# The test system is allowed to run these tests before the rest of Qt has
# been compiled.
#
TEMPLATE=subdirs
SUBDIRS=\
    gui \
    qopengl \
    qtransformedscreen \
    qwindowsurface \
    qwsembedwidget \
    qwsinputmethod \
    qwswindowsystem \
    qx11info \

# This test cannot be run on Mac OS
mac*:SUBDIRS -= \
    qwindowsurface \

# These tests are only valid for QWS
!embedded|wince*:SUBDIRS -= \
    qtransformedscreen \
    qwsembedwidget \
    qwsinputmethod \
    qwswindowsystem \

