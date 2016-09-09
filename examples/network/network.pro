requires(qtHaveModule(network))

TEMPLATE      = subdirs
QT_FOR_CONFIG += network-private
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
        !vxworks:!qnx:!winrt:!integrity:!uikit: SUBDIRS += network-chat

        SUBDIRS += \
                bearermonitor \
                fortuneclient \
                fortuneserver

    }

    qtConfig(openssl): SUBDIRS += securesocketclient
    qtConfig(openssl-linked): SUBDIRS += securesocketclient
    qtConfig(sctp): SUBDIRS += multistreamserver multistreamclient
}

EXAMPLE_FILES = shared
