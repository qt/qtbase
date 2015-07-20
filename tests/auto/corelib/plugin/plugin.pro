TEMPLATE=subdirs
SUBDIRS=\
    qfactoryloader \
    quuid

load(qfeatures)
!contains(QT_DISABLED_FEATURES, library): SUBDIRS += \
    qpluginloader \
    qplugin \
    qlibrary
