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

    qtConfig(processenvironment): SUBDIRS += network-chat

    SUBDIRS += \
            fortuneclient \
            fortuneserver

    qtConfig(ssl): SUBDIRS += securesocketclient
    qtConfig(dtls): SUBDIRS += secureudpserver secureudpclient
    qtConfig(sctp): SUBDIRS += multistreamserver multistreamclient
}

EXAMPLE_FILES = shared
