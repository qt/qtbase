TEMPLATE = subdirs
SUBDIRS += trace
!wince*:contains(QT_CONFIG, opengl):SUBDIRS += opengl
contains(QT_CONFIG, openvg):contains(QT_CONFIG, egl) {
    SUBDIRS += openvg
}

contains(QT_CONFIG, shivavg) {
    # Only works under X11 at present
    !win32:!embedded:!mac:SUBDIRS += shivavg
}

!win32:!embedded:!mac:!symbian:CONFIG += x11

x11:contains(QT_CONFIG, opengles2):contains(QT_CONFIG, egl):SUBDIRS += meego
