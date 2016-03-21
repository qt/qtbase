TEMPLATE=subdirs
SUBDIRS=\
    qfactoryloader \
    quuid

load(qfeatures)
!contains(QT_DISABLED_FEATURES, library): SUBDIRS += \
    qpluginloader \
    qplugin \
    qlibrary

contains(CONFIG, static) {
    message(Disabling tests requiring shared build of Qt)
    SUBDIRS -= qfactoryloader \
               qpluginloader
}
