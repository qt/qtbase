TEMPLATE=subdirs

SUBDIRS = bearerex \
filetest \
foreignwindows \
gestures \
inputmethodhints \
keypadnavigation \
lance \
network_remote_stresstest \
network_stresstest \
qcursor \
qdesktopservices \
qdesktopwidget \
qgraphicsitem \
qgraphicsitemgroup \
qgraphicslayout/flicker \
qhttpnetworkconnection \
qimagereader \
qlayout \
qlocale \
qmimedatabase \
qnetworkaccessmanager/qget \
qnetworkconfigurationmanager \
qnetworkconfiguration \
qnetworkreply \
qstorageinfo \
qscreen \
qssloptions \
qsslsocket \
qsysinfo \
qtabletevent \
qtexteditlist \
qtbug-8933 \
qtbug-52641 \
qtouchevent \
touch \
qwidget_zorder \
repaint \
socketengine \
textrendering \
widgets \
windowflags \
windowgeometry \
windowmodality \
widgetgrab \
xembed-raster \
xembed-widgets \
shortcuts \
dialogs \
windowtransparency \
unc

!contains(QT_CONFIG, openssl):!contains(QT_CONFIG, openssl-linked):SUBDIRS -= qssloptions

contains(QT_CONFIG, opengl) {
    SUBDIRS += qopengltextureblitter
    contains(QT_CONFIG, egl): SUBDIRS += qopenglcontext
}

win32 {
    SUBDIRS -= network_remote_stresstest network_stresstest
    # disable some tests on wince because of missing dependencies
    wince: SUBDIRS -= lance windowmodality
}

lessThan(QT_MAJOR_VERSION, 5): SUBDIRS -= bearerex lance qnetworkaccessmanager/qget qmimedatabase qnetworkreply \
qpainfo qscreen  socketengine xembed-raster xembed-widgets windowtransparency \
foreignwindows
