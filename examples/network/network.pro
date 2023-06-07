requires(qtHaveModule(network))

TEMPLATE      = subdirs
QT_FOR_CONFIG += network-private
!integrity: SUBDIRS += dnslookup
qtHaveModule(widgets) {
    SUBDIRS +=  \
                blockingfortuneclient \
                broadcastreceiver \
                broadcastsender \
                fortuneclient \
                fortuneserver \
                http \
                multicastreceiver \
                multicastsender \
                rsslisting \
                threadedfortuneserver \
                torrent

    qtConfig(processenvironment): SUBDIRS += network-chat
    qtConfig(ssl): SUBDIRS += securesocketclient
    qtConfig(dtls): SUBDIRS += secureudpserver secureudpclient
    qtConfig(sctp): SUBDIRS += multistreamserver multistreamclient
}

EXAMPLE_FILES = shared
