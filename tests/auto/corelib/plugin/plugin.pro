TEMPLATE=subdirs
SUBDIRS=\
    qfactoryloader \
    qlibrary \
    qplugin \
    qpluginloader \
    quuid

contains(CONFIG, static) {
    message(Disabling tests requiring shared build of Qt)
    SUBDIRS -= qfactoryloader \
               qpluginloader
}
