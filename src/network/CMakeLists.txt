# Generated from network.pro.

#####################################################################
## QtNetwork Module:
#####################################################################

add_qt_module(Network
    SOURCES
        access/qabstractnetworkcache.cpp access/qabstractnetworkcache.h access/qabstractnetworkcache_p.h
        access/qhsts.cpp access/qhsts_p.h
        access/qhstspolicy.cpp access/qhstspolicy.h
        access/qhstsstore.cpp access/qhstsstore_p.h
        access/qnetworkaccessauthenticationmanager.cpp access/qnetworkaccessauthenticationmanager_p.h
        access/qnetworkaccessbackend.cpp access/qnetworkaccessbackend_p.h
        access/qnetworkaccesscache.cpp access/qnetworkaccesscache_p.h
        access/qnetworkaccesscachebackend.cpp access/qnetworkaccesscachebackend_p.h
        access/qnetworkaccessdebugpipebackend.cpp access/qnetworkaccessdebugpipebackend_p.h
        access/qnetworkaccessfilebackend.cpp access/qnetworkaccessfilebackend_p.h
        access/qnetworkaccessmanager.cpp access/qnetworkaccessmanager.h access/qnetworkaccessmanager_p.h
        access/qnetworkcookie.cpp access/qnetworkcookie.h access/qnetworkcookie_p.h
        access/qnetworkcookiejar.cpp access/qnetworkcookiejar.h access/qnetworkcookiejar_p.h
        access/qnetworkfile.cpp access/qnetworkfile_p.h
        access/qnetworkreply.cpp access/qnetworkreply.h access/qnetworkreply_p.h
        access/qnetworkreplydataimpl.cpp access/qnetworkreplydataimpl_p.h
        access/qnetworkreplyfileimpl.cpp access/qnetworkreplyfileimpl_p.h
        access/qnetworkreplyimpl.cpp access/qnetworkreplyimpl_p.h
        access/qnetworkrequest.cpp access/qnetworkrequest.h access/qnetworkrequest_p.h
        bearer/qbearerengine.cpp bearer/qbearerengine_p.h
        bearer/qbearerplugin.cpp bearer/qbearerplugin_p.h
        bearer/qnetworkconfigmanager.cpp bearer/qnetworkconfigmanager.h bearer/qnetworkconfigmanager_p.cpp bearer/qnetworkconfigmanager_p.h
        bearer/qnetworkconfiguration.cpp bearer/qnetworkconfiguration.h bearer/qnetworkconfiguration_p.h
        bearer/qnetworksession.cpp bearer/qnetworksession.h bearer/qnetworksession_p.h
        bearer/qsharednetworksession.cpp bearer/qsharednetworksession_p.h
        kernel/qauthenticator.cpp kernel/qauthenticator.h kernel/qauthenticator_p.h
        kernel/qhostaddress.cpp kernel/qhostaddress.h kernel/qhostaddress_p.h
        kernel/qhostinfo.cpp kernel/qhostinfo.h kernel/qhostinfo_p.h
        kernel/qnetworkdatagram.cpp kernel/qnetworkdatagram.h kernel/qnetworkdatagram_p.h
        kernel/qnetworkinterface.cpp kernel/qnetworkinterface.h kernel/qnetworkinterface_p.h
        kernel/qnetworkinterface_unix_p.h
        kernel/qnetworkproxy.cpp kernel/qnetworkproxy.h
        kernel/qtnetworkglobal.h kernel/qtnetworkglobal_p.h
        socket/qabstractsocket.cpp socket/qabstractsocket.h socket/qabstractsocket_p.h
        socket/qabstractsocketengine.cpp socket/qabstractsocketengine_p.h
        socket/qtcpserver.cpp socket/qtcpserver.h socket/qtcpserver_p.h
        socket/qtcpsocket.cpp socket/qtcpsocket.h socket/qtcpsocket_p.h
        socket/qudpsocket.cpp socket/qudpsocket.h
        ssl/qasn1element.cpp ssl/qasn1element_p.h
        ssl/qpassworddigestor.cpp ssl/qpassworddigestor.h
        ssl/qssl.cpp ssl/qssl.h ssl/qssl_p.h
        ssl/qsslcertificate.cpp ssl/qsslcertificate.h ssl/qsslcertificate_p.h
        ssl/qsslcertificateextension.cpp ssl/qsslcertificateextension.h ssl/qsslcertificateextension_p.h
    DEFINES
        QT_NO_USING_NAMESPACE
        QT_NO_FOREACH
    INCLUDE_DIRECTORIES
        kernel
    LIBRARIES
        Qt::CorePrivate
)

## Scopes:
#####################################################################

find_library(FWCoreFoundation CoreFoundation)
find_library(FWSecurity Security)
find_library(FWCFNetwork CFNetwork)
find_library(FWCoreServices CoreServices)
find_library(FWSystemConfiguration SystemConfiguration)

#
#extend_target(Network CONDITION MSVC
#)
#
#extend_target(Network CONDITION (MSVC) AND (equals(QT_ARCH, i386))
#)
#
#extend_target(Network CONDITION QT_FEATURE_bearermanagement
#)

extend_target(Network CONDITION QT_FEATURE_ftp
    SOURCES
        access/qftp.cpp access/qftp_p.h
        access/qnetworkaccessftpbackend.cpp access/qnetworkaccessftpbackend_p.h
)

extend_target(Network CONDITION QT_FEATURE_networkdiskcache
    SOURCES
        access/qnetworkdiskcache.cpp access/qnetworkdiskcache.h access/qnetworkdiskcache_p.h
)

extend_target(Network CONDITION APPLE_OSX
    LIBRARIES
        ${FWSecurity}
)

extend_target(Network CONDITION WASM
    SOURCES
        access/qnetworkreplywasmimpl.cpp access/qnetworkreplywasmimpl_p.h
)

extend_target(Network CONDITION QT_FEATURE_http
    SOURCES
        access/http2/bitstreams.cpp access/http2/bitstreams_p.h
        access/http2/hpack.cpp access/http2/hpack_p.h
        access/http2/hpacktable.cpp access/http2/hpacktable_p.h
        access/http2/http2frames.cpp access/http2/http2frames_p.h
        access/http2/http2protocol.cpp access/http2/http2protocol_p.h
        access/http2/http2streams.cpp access/http2/http2streams_p.h
        access/http2/huffman.cpp access/http2/huffman_p.h
        access/qabstractprotocolhandler.cpp access/qabstractprotocolhandler_p.h
        access/qhttp2protocolhandler.cpp access/qhttp2protocolhandler_p.h
        access/qhttpmultipart.cpp access/qhttpmultipart.h access/qhttpmultipart_p.h
        access/qhttpnetworkconnection.cpp access/qhttpnetworkconnection_p.h
        access/qhttpnetworkconnectionchannel.cpp access/qhttpnetworkconnectionchannel_p.h
        access/qhttpnetworkheader.cpp access/qhttpnetworkheader_p.h
        access/qhttpnetworkreply.cpp access/qhttpnetworkreply_p.h
        access/qhttpnetworkrequest.cpp access/qhttpnetworkrequest_p.h
        access/qhttpprotocolhandler.cpp access/qhttpprotocolhandler_p.h
        access/qhttpthreaddelegate.cpp access/qhttpthreaddelegate_p.h
        access/qnetworkreplyhttpimpl.cpp access/qnetworkreplyhttpimpl_p.h
    LIBRARIES
        ZLIB::ZLIB
)

extend_target(Network CONDITION APPLE_UIKIT
    SOURCES
        kernel/qnetworkinterface_uikit_p.h
)

if(APPLE_OSX)
    set(proxy_sources kernel/qnetworkproxy_mac.cpp)
elseif(WIN32 AND NOT WINRT)
    set(proxy_sources kernel/qnetworkproxy_win.cpp)
elseif(QT_FEATURE_libproxy)
    set(proxy_sources kernel/qnetworkproxy_libproxy.cpp)
    set(proxy_libs PkgConfig::Libproxy dl)
else()
    set(proxy_sources kernel/qnetworkproxy_generic.cpp)
endif()

if(NOT DEFINED proxy_libs)
    set(proxy_libs "")
endif()

extend_target(Network
    SOURCES
        ${proxy_sources}
    LIBRARIES
        ${proxy_libs}
)

extend_target(Network CONDITION (QT_FEATURE_http) AND (QT_FEATURE_ssl)
    SOURCES
        access/qspdyprotocolhandler.cpp access/qspdyprotocolhandler_p.h
)

extend_target(Network CONDITION QT_FEATURE_ftp
    SOURCES
        kernel/qurlinfo.cpp kernel/qurlinfo_p.h
)

extend_target(Network CONDITION QT_FEATURE_dnslookup
    SOURCES
        kernel/qdnslookup.cpp kernel/qdnslookup.h kernel/qdnslookup_p.h
)

extend_target(Network CONDITION UNIX
    SOURCES
        kernel/qhostinfo_unix.cpp
)
#
#extend_target(Network CONDITION (UNIX) AND (NOT INTEGRITY)
#)

extend_target(Network CONDITION WIN32
    SOURCES
        kernel/qhostinfo_win.cpp
)

extend_target(Network CONDITION WIN32 AND NOT WINRT
    SOURCES
        kernel/qnetworkinterface_win.cpp
    LIBRARIES
        dnsapi
        iphlpapi
)

extend_target(Network CONDITION WIN32 AND NOT WINRT AND QT_FEATURE_dnslookup
    SOURCES
        kernel/qdnslookup_win.cpp
)

extend_target(Network CONDITION ((UNIX) AND (NOT INTEGRITY)) AND (QT_FEATURE_dnslookup)
    SOURCES
        kernel/qdnslookup_unix.cpp
)

extend_target(Network CONDITION (UNIX) AND (QT_FEATURE_linux_netlink)
    SOURCES
        kernel/qnetworkinterface_linux.cpp
)

extend_target(Network CONDITION (UNIX) AND (NOT ((UNIX) AND (QT_FEATURE_linux_netlink)))
    SOURCES
        kernel/qnetworkinterface_unix.cpp
)

# TODO: how to merge to above
extend_target(Network CONDITION APPLE_OSX
    SOURCES
        kernel/qnetworkinterface_unix.cpp
)
#
#extend_target(Network CONDITION ANDROID
#)

extend_target(Network CONDITION (ANDROID) AND (QT_FEATURE_dnslookup)
    SOURCES
        kernel/qdnslookup_unix.cpp
        kernel/qdnslookup_android.cpp
)

extend_target(Network CONDITION QT_FEATURE_socks5
    SOURCES
        socket/qsocks5socketengine.cpp socket/qsocks5socketengine_p.h
)

extend_target(Network CONDITION QT_FEATURE_http
    SOURCES
        socket/qhttpsocketengine.cpp socket/qhttpsocketengine_p.h
)

extend_target(Network CONDITION QT_FEATURE_sctp
    SOURCES
        socket/qsctpserver.cpp socket/qsctpserver.h socket/qsctpserver_p.h
        socket/qsctpsocket.cpp socket/qsctpsocket.h socket/qsctpsocket_p.h
)

extend_target(Network CONDITION NOT WINRT
    SOURCES
        socket/qnativesocketengine.cpp socket/qnativesocketengine_p.h
)

extend_target(Network CONDITION UNIX
    SOURCES
        socket/qnativesocketengine_unix.cpp
        socket/qnet_unix_p.h
)

### FIXME
# extend_target(Network CONDITION msvc AND equals(QT_ARCH, i386):
#     QMAKE_LFLAGS += /BASE:0x64000000
# )

extend_target(Network CONDITION WIN32
    SOURCES
)

extend_target(Network CONDITION (WIN32) AND (NOT WINRT)
    SOURCES
        socket/qnativesocketengine_win.cpp
)

extend_target(Network CONDITION WIN32
    LIBRARIES
        ws2_32
)

extend_target(Network CONDITION (WIN32) AND (NOT WINRT)
    LIBRARIES
        advapi32
)

extend_target(Network CONDITION WINRT
    SOURCES
        socket/qnativesocketengine_winrt.cpp socket/qnativesocketengine_winrt_p.h
)

extend_target(Network CONDITION QT_FEATURE_localserver
    SOURCES
        socket/qlocalserver.cpp socket/qlocalserver.h socket/qlocalserver_p.h
        socket/qlocalsocket.cpp socket/qlocalsocket.h socket/qlocalsocket_p.h
)

extend_target(Network CONDITION QT_FEATURE_localserver AND ((INTEGRITY) OR (WINRT))
    SOURCES
        socket/qlocalsocket_tcp.cpp
        socket/qlocalserver_tcp.cpp
    DEFINES
        QT_LOCALSOCKET_TCP
)

extend_target(Network CONDITION QT_FEATURE_localserver AND (UNIX) AND (NOT INTEGRITY)
    SOURCES
        socket/qlocalsocket_unix.cpp
        socket/qlocalserver_unix.cpp
)

extend_target(Network CONDITION QT_FEATURE_localserver AND (WIN32) AND (NOT WINRT)
    SOURCES
        socket/qlocalsocket_win.cpp
        socket/qlocalserver_win.cpp
)

extend_target(Network CONDITION NOT QT_FEATURE_openssl
    SOURCES
        ssl/qsslcertificate_qt.cpp
)

extend_target(Network CONDITION QT_FEATURE_ssl
    SOURCES
        ssl/qsslcipher.cpp ssl/qsslcipher.h ssl/qsslcipher_p.h
        ssl/qsslconfiguration.cpp ssl/qsslconfiguration.h ssl/qsslconfiguration_p.h
        ssl/qssldiffiehellmanparameters.cpp ssl/qssldiffiehellmanparameters.h ssl/qssldiffiehellmanparameters_p.h
        ssl/qsslellipticcurve.cpp ssl/qsslellipticcurve.h
        ssl/qsslerror.cpp ssl/qsslerror.h
        ssl/qsslkey.h ssl/qsslkey_p.cpp ssl/qsslkey_p.h
        ssl/qsslpresharedkeyauthenticator.cpp ssl/qsslpresharedkeyauthenticator.h ssl/qsslpresharedkeyauthenticator_p.h
        ssl/qsslsocket.cpp ssl/qsslsocket.h ssl/qsslsocket_p.h
)

extend_target(Network CONDITION (QT_FEATURE_ssl) AND (WINRT)
    SOURCES
        ssl/qsslcertificate_winrt.cpp
        ssl/qssldiffiehellmanparameters_dummy.cpp
        ssl/qsslellipticcurve_dummy.cpp
        ssl/qsslkey_qt.cpp
        ssl/qsslkey_winrt.cpp
        ssl/qsslsocket_winrt.cpp ssl/qsslsocket_winrt_p.h
)

extend_target(Network CONDITION (QT_FEATURE_ssl) AND (QT_FEATURE_securetransport)
    SOURCES
        ssl/qssldiffiehellmanparameters_dummy.cpp
        ssl/qsslellipticcurve_dummy.cpp
        ssl/qsslkey_mac.cpp
        ssl/qsslkey_qt.cpp
        ssl/qsslsocket_mac.cpp ssl/qsslsocket_mac_p.h
        ssl/qsslsocket_mac_shared.cpp
        ssl/qsslsocket_qt.cpp
)

extend_target(Network CONDITION (QT_FEATURE_ssl) AND (QT_FEATURE_dtls)
    SOURCES
        ssl/qdtls.cpp ssl/qdtls.h ssl/qdtls_p.h
)

extend_target(Network CONDITION (QT_FEATURE_ssl) AND (QT_FEATURE_openssl)
    SOURCES
        ssl/qsslcertificate_openssl.cpp
        ssl/qsslcontext_openssl.cpp ssl/qsslcontext_openssl_p.h
        ssl/qssldiffiehellmanparameters_openssl.cpp
        ssl/qsslellipticcurve_openssl.cpp
        ssl/qsslkey_openssl.cpp
        ssl/qsslsocket_openssl.cpp ssl/qsslsocket_openssl_p.h
        ssl/qsslsocket_openssl_symbols.cpp ssl/qsslsocket_openssl_symbols_p.h
)

extend_target(Network CONDITION ((QT_FEATURE_ssl) AND (QT_FEATURE_openssl)) AND (QT_FEATURE_dtls)
    SOURCES
        ssl/qdtls_openssl.cpp ssl/qdtls_openssl_p.h
)

extend_target(Network CONDITION ((QT_FEATURE_ssl) AND (QT_FEATURE_openssl)) AND (QT_FEATURE_opensslv11)
    SOURCES
        ssl/qsslcontext_openssl11.cpp
        ssl/qsslsocket_openssl11.cpp
        ssl/qsslsocket_openssl11_symbols_p.h
)

extend_target(Network CONDITION ((QT_FEATURE_ssl) AND (QT_FEATURE_openssl)) AND (NOT QT_FEATURE_opensslv11)
    SOURCES
        ssl/qsslcontext_opensslpre11.cpp
        ssl/qsslsocket_opensslpre11.cpp
        ssl/qsslsocket_opensslpre11_symbols_p.h
)

extend_target(Network CONDITION (APPLE_OSX) AND (NOT APPLE_UIKIT)
    LIBRARIES
        ${FWCoreServices}
        ${FWSystemConfiguration}
)

extend_target(Network CONDITION APPLE_OSX
    LIBRARIES
        ${FWCoreFoundation}
)

extend_target(Network CONDITION ((QT_FEATURE_ssl) AND (QT_FEATURE_openssl)) AND (APPLE)
    SOURCES
        ssl/qsslsocket_mac_shared.cpp
)
#
#extend_target(Network CONDITION ((QT_FEATURE_ssl) AND (QT_FEATURE_openssl)) AND (ANDROID)
#)

extend_target(Network CONDITION (((QT_FEATURE_ssl) AND (QT_FEATURE_openssl)) AND (ANDROID)) AND (NOT ANDROID_EMBEDDED)
    SOURCES
        ssl/qsslsocket_openssl_android.cpp
)
#
#extend_target(Network CONDITION ((QT_FEATURE_ssl) AND (QT_FEATURE_openssl)) AND (QT_FEATURE_openssl_linked)
#)
#
#extend_target(Network CONDITION ((QT_FEATURE_ssl) AND (QT_FEATURE_openssl)) AND (NOT (((QT_FEATURE_ssl) AND (QT_FEATURE_openssl)) AND (QT_FEATURE_openssl_linked)))
#)

extend_target(Network CONDITION ((QT_FEATURE_ssl) AND (QT_FEATURE_openssl)) AND (WIN32)
    SOURCES
        ssl/qwindowscarootfetcher.cpp ssl/qwindowscarootfetcher_p.h
    LIBRARIES
        crypt32
)

add_qt_docs(./doc/qtnetwork.qdocconf)
