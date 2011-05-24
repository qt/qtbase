TEMPLATE      = subdirs
SUBDIRS       = \
                download \
                downloadmanager

!contains(QT_CONFIG, no-gui) {
    SUBDIRS +=  \
                blockingfortuneclient \
                broadcastreceiver \
                broadcastsender \
                fortuneclient \
                fortuneserver \
                qftp \
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

symbian: SUBDIRS = qftp

# install
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS network.pro README
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/network
INSTALLS += sources

symbian: CONFIG += qt_example
maemo5: CONFIG += qt_example
