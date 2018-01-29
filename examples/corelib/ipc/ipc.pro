requires(qtHaveModule(widgets))

TEMPLATE      = subdirs

qtConfig(sharedmemory): SUBDIRS = sharedmemory
qtHaveModule(network) {
    QT_FOR_CONFIG += network

    qtConfig(localserver): SUBDIRS += localfortuneserver localfortuneclient
}
