TEMPLATE=subdirs
QT_FOR_CONFIG += network-private gui-private

SUBDIRS = \
filetest \
embeddedintoforeignwindow \
foreignwindows \
fontfeatures \
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
qscreen_xrandr \
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
xmlstreamlint \
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

qtConfig(xcb): SUBDIRS += xembed
