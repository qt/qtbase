TEMPLATE  = subdirs
SUBDIRS   = styledemo raycasting flickable digiflip

contains(QT_CONFIG, svg) {
    SUBDIRS += embeddedsvgviewer \
               desktopservices
    fluidlauncher.subdir = fluidlauncher
    fluidlauncher.depends = styledemo desktopservices raycasting flickable digiflip lightmaps flightinfo
    !vxworks:!qnx:SUBDIRS += fluidlauncher
}

SUBDIRS += lightmaps
SUBDIRS += flightinfo
contains(QT_CONFIG, svg) {
    SUBDIRS += weatherinfo
}

contains(QT_CONFIG, webkit) {
    SUBDIRS += anomaly
}

contains(QT_CONFIG, declarative) {
    # Qml demos require DEPLOYMENT support. Therefore, only symbian.
    symbian:SUBDIRS += qmlcalculator qmlclocks qmldialcontrol qmleasing qmlflickr qmlphotoviewer qmltwitter
}

# install
sources.files = README *.pro
sources.path = $$[QT_INSTALL_DEMOS]/qtbase/embedded
INSTALLS += sources

symbian: CONFIG += qt_demo
