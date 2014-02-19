requires(qtHaveModule(network))

TEMPLATE      = subdirs
SUBDIRS       = \
                dnslookup \
                download \
                downloadmanager

qtHaveModule(widgets) {
    SUBDIRS +=  \
                blockingfortuneclient \
                broadcastreceiver \
                broadcastsender \
                fortuneclient \
                fortuneserver \
                http \
                loopback \
                threadedfortuneserver \
                googlesuggest \
                torrent \
                bearermonitor \
                multicastreceiver \
                multicastsender

    # no QProcess
    !vxworks:!qnx:!winrt:SUBDIRS += network-chat

    contains(QT_CONFIG, openssl):SUBDIRS += securesocketclient
    contains(QT_CONFIG, openssl-linked):SUBDIRS += securesocketclient
}
