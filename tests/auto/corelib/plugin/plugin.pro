TEMPLATE=subdirs
SUBDIRS=\
    qfactoryloader \
    quuid

qtConfig(library): SUBDIRS += \
    qpluginloader \
    qplugin \
    qlibrary

contains(CONFIG, static) {
    message(Disabling tests requiring shared build of Qt)
    SUBDIRS -= qfactoryloader \
               qplugin \
               qpluginloader
}
