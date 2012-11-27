TEMPLATE      = subdirs
SUBDIRS       = \
                dnslookup \
                download \
                downloadmanager

!contains(QT_CONFIG, no-widgets) {
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
    !vxworks:!qnx:SUBDIRS += network-chat

    contains(QT_CONFIG, openssl):SUBDIRS += securesocketclient
    contains(QT_CONFIG, openssl-linked):SUBDIRS += securesocketclient
}
