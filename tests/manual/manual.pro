TEMPLATE=subdirs

SUBDIRS = bearerex \
filetest \
gestures \
inputmethodhints \
keypadnavigation \
lance \
network_remote_stresstest \
network_stresstest \
qcursor \
qdesktopwidget \
qgraphicsitem \
qgraphicsitemgroup \
qgraphicslayout/flicker \
qhttpnetworkconnection \
qimagereader \
qlayout \
qlocale \
qnetworkaccessmanager/qget \
qnetworkconfigurationmanager \
qnetworkreply \
qscreen \
qssloptions \
qtabletevent \
qtbug-8933 \
qtouchevent \
qwidget_zorder \
repaint \
socketengine \
textrendering \
widgets/itemviews/delegate \
windowflags \
windowgeometry \
windowmodality \
widgetgrab \
dialogs

!contains(QT_CONFIG, openssl):!contains(QT_CONFIG, openssl-linked):SUBDIRS -= qssloptions

# disable some tests on wince because of missing dependencies
wince*:SUBDIRS -= \
    lance windowmodality \
    network_remote_stresstest network_stresstest
