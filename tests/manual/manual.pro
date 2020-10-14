TEMPLATE=subdirs
QT_FOR_CONFIG += network-private gui-private

SUBDIRS = \
filetest \
embeddedintoforeignwindow \
foreignwindows \
gestures \
highdpi \
inputmethodhints \
keypadnavigation \
lance \
network_remote_stresstest \
network_stresstest \
qcursor \
qdesktopservices \
qgraphicsitem \
qgraphicsitemgroup \
qgraphicslayout/flicker \
qhttpnetworkconnection \
qimagereader \
qlayout \
qlocale \
qmimedatabase \
qnetconmonitor \
qnetworkaccessmanager/qget \
qnetworkreply \
qstorageinfo \
qscreen \
qssloptions \
qsslsocket \
qsysinfo \
qtabletevent \
qtexteditlist \
qtexttableborders \
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
unc \
qtabbar \
rhi

!qtConfig(openssl): SUBDIRS -= qssloptions

qtConfig(opengl) {
    SUBDIRS += qopengltextureblitter
    qtConfig(egl): SUBDIRS += qopenglcontext
}

win32: SUBDIRS -= network_remote_stresstest network_stresstest

qtConfig(vulkan): SUBDIRS += qvulkaninstance
