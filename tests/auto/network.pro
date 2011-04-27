# The tests in this .pro file _MUST_ use QtCore and QtNetwork only
# (i.e. QT=core network).
# The test system is allowed to run these tests before the rest of Qt has
# been compiled.
TEMPLATE=subdirs
SUBDIRS=\
    networkselftest \
    qabstractnetworkcache \
    qabstractsocket \
    qauthenticator \
    qeventloop \
    qftp \
    qhostaddress \
    qhostinfo \
    qhttp \
    qhttpnetworkconnection \
    qhttpnetworkreply \
    qhttpsocketengine \
    platformsocketengine \
    qnetworkaccessmanager \
    qnetworkaddressentry \
    qnetworkcachemetadata \
    qnetworkconfiguration \
    qnetworkconfigurationmanager \
    qnetworkcookie \
    qnetworkcookiejar \
    qnetworkdiskcache \
    qnetworkinterface \
    qnetworkproxy \
    qnetworkreply \
    qnetworkrequest \
    qnetworksession \
    qobjectperformance \
    qsocketnotifier \
    qsocks5socketengine \
    qsslcertificate \
    qsslcipher \
    qsslerror \
    qsslkey \
    qsslsocket \
    qsslsocket_onDemandCertificates_member \
    qsslsocket_onDemandCertificates_static \
    qtcpserver \
    qudpsocket \
#    qnetworkproxyfactory \ # Uses a hardcoded proxy configuration

!contains(QT_CONFIG, private_tests): SUBDIRS -= \
    qauthenticator \
    qhttpnetworkconnection \
    qhttpnetworkreply \
    platformsocketengine \
    qsocketnotifier \
    qsocks5socketengine \

