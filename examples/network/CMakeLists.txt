# Generated from network.pro.

add_subdirectory(download)
add_subdirectory(downloadmanager)

if(NOT INTEGRITY)
    add_subdirectory(dnslookup)
endif()

if(TARGET Qt::Widgets)
    add_subdirectory(blockingfortuneclient)
    add_subdirectory(broadcastreceiver)
    add_subdirectory(broadcastsender)
    add_subdirectory(http)
    add_subdirectory(loopback)
    add_subdirectory(threadedfortuneserver)
    add_subdirectory(googlesuggest)
    # add_subdirectory(torrent) FIXME: This does not find its ui file:-/
    add_subdirectory(multicastreceiver)
    add_subdirectory(multicastsender)

    if(QT_FEATURE_bearermanagement)
        add_subdirectory(fortuneclient)
        add_subdirectory(fortuneserver)

        if(QT_FEATURE_processenvironment)
            add_subdirectory(network-chat)
        endif()
    endif()

    if(QT_FEATURE_openssl)
        add_subdirectory(securesocketclient)
    endif()

    if(QT_FEATURE_dtls)
        add_subdirectory(secureudpserver)
        add_subdirectory(secureudpclient)
    endif()

    if(QT_FEATURE_sctp)
        add_subdirectory(multistreamserver)
        add_subdirectory(multistreamclient)
    endif()
endif()
