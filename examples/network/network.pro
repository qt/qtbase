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

    !emscripten:qtConfig(bearermanagement) {
        qtConfig(processenvironment): SUBDIRS += network-chat

        SUBDIRS += \
                bearermonitor \
                fortuneclient \
                fortuneserver

    }

    !emscripten:qtConfig(openssl): SUBDIRS += securesocketclient
    !emscripten:qtConfig(openssl-linked): SUBDIRS += securesocketclient
    !emscripten:qtConfig(sctp): SUBDIRS += multistreamserver multistreamclient
}

EXAMPLE_FILES = shared
