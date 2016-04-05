requires(qtHaveModule(network))

TEMPLATE      = subdirs
SUBDIRS       = \
                download \
                downloadmanager
!integrity: SUBDIRS += dnslookup
qtHaveModule(widgets) {
    SUBDIRS +=  \
                blockingfortuneclient \
                broadcastreceiver \
                broadcastsender \
                http \
                loopback \
                threadedfortuneserver \
                googlesuggest \
                torrent \
                multicastreceiver \
                multicastsender

    load(qfeatures)
    !contains(QT_DISABLED_FEATURES, bearermanagement) {
        # no QProcess
        !vxworks:!qnx:!winrt:!integrity: SUBDIRS += network-chat

        SUBDIRS += \
                bearermonitor \
                fortuneclient \
                fortuneserver

    }

    contains(QT_CONFIG, openssl):SUBDIRS += securesocketclient
    contains(QT_CONFIG, openssl-linked):SUBDIRS += securesocketclient
}
