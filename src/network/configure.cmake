#### Inputs



#### Libraries

qt_find_package(WrapBrotli PROVIDED_TARGETS WrapBrotli::WrapBrotliDec MODULE_NAME network QMAKE_LIB brotli)
qt_find_package(Libproxy PROVIDED_TARGETS PkgConfig::Libproxy MODULE_NAME network QMAKE_LIB libproxy)
qt_find_package(WrapOpenSSLHeaders PROVIDED_TARGETS WrapOpenSSLHeaders::WrapOpenSSLHeaders MODULE_NAME network)
# openssl_headers
qt_config_compile_test(openssl_headers
    LIBRARIES
        WrapOpenSSLHeaders::WrapOpenSSLHeaders
    CODE
"#include <openssl/ssl.h>
#include <openssl/opensslv.h>
#if !defined(OPENSSL_VERSION_NUMBER) || OPENSSL_VERSION_NUMBER-0 < 0x10101000L
#  error OpenSSL >= 1.1.1 is required
#endif
#if !defined(OPENSSL_NO_EC) && !defined(SSL_CTRL_SET_CURVES)
#  error OpenSSL was reported as >= 1.1.1 but is missing required features, possibly it is libressl which is unsupported
#endif

int main(void)
{
    /* BEGIN TEST: */
    /* END TEST: */
    return 0;
}
")

qt_find_package(WrapOpenSSL PROVIDED_TARGETS WrapOpenSSL::WrapOpenSSL MODULE_NAME network QMAKE_LIB openssl)
# openssl
qt_config_compile_test(openssl
    LIBRARIES
        WrapOpenSSL::WrapOpenSSL
    CODE
"#include <openssl/ssl.h>
#include <openssl/opensslv.h>
#if !defined(OPENSSL_VERSION_NUMBER) || OPENSSL_VERSION_NUMBER-0 < 0x10101000L
#  error OpenSSL >= 1.1.1 is required
#endif
#if !defined(OPENSSL_NO_EC) && !defined(SSL_CTRL_SET_CURVES)
#  error OpenSSL was reported as >= 1.1.1 but is missing required features, possibly it is libressl which is unsupported
#endif

int main(void)
{
    /* BEGIN TEST: */
SSL_free(SSL_new(0));
    /* END TEST: */
    return 0;
}
")

qt_find_package(GSSAPI PROVIDED_TARGETS GSSAPI::GSSAPI MODULE_NAME network QMAKE_LIB gssapi)


#### Tests

# getifaddrs
qt_config_compile_test(getifaddrs
    LABEL "getifaddrs()"
    CODE
"#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <ifaddrs.h>

int main(void)
{
    /* BEGIN TEST: */
ifaddrs *list;
getifaddrs(&list);
freeifaddrs(list);
    /* END TEST: */
    return 0;
}
"# FIXME: use: unmapped library: network
)

# ifr_index
qt_config_compile_test(ifr_index
    LABEL "ifr_index"
    CODE
"#include <net/if.h>

int main(void)
{
    /* BEGIN TEST: */
struct ifreq req;
req.ifr_index = 0;
    /* END TEST: */
    return 0;
}
")

# ipv6ifname
qt_config_compile_test(ipv6ifname
    LABEL "IPv6 ifname"
    CODE
"#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>

int main(void)
{
    /* BEGIN TEST: */
char buf[IFNAMSIZ];
if_nametoindex(\"eth0\");
if_indextoname(1, buf);
if_freenameindex(if_nameindex());
    /* END TEST: */
    return 0;
}
"# FIXME: use: unmapped library: network
)

# linux-netlink
qt_config_compile_test(linux_netlink
    LABEL "Linux AF_NETLINK sockets"
    CODE
"#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>

int main(void)
{
    /* BEGIN TEST: */
struct rtattr rta = { };
struct ifinfomsg ifi = {};
struct ifaddrmsg ifa = {};
struct ifa_cacheinfo ci;
ci.ifa_prefered = ci.ifa_valid = 0;
(void)RTM_NEWLINK; (void)RTM_NEWADDR;
(void)IFLA_ADDRESS; (void)IFLA_IFNAME;
(void)IFA_ADDRESS; (void)IFA_LABEL; (void)IFA_CACHEINFO;
(void)(IFA_F_SECONDARY | IFA_F_DEPRECATED | IFA_F_PERMANENT | IFA_F_MANAGETEMPADDR);
    /* END TEST: */
    return 0;
}
")

# sctp
qt_config_compile_test(sctp
    LABEL "SCTP support"
    CODE
"#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>

int main(void)
{
    /* BEGIN TEST: */
sctp_initmsg sctpInitMsg;
socklen_t sctpInitMsgSize = sizeof(sctpInitMsg);
(void) socket(PF_INET, SOCK_STREAM, IPPROTO_SCTP);
(void) getsockopt(-1, SOL_SCTP, SCTP_INITMSG, &sctpInitMsg, &sctpInitMsgSize);
    /* END TEST: */
    return 0;
}
"# FIXME: use: unmapped library: network
)

# dtls
qt_config_compile_test(dtls
    LABEL "DTLS support in OpenSSL"
    LIBRARIES
        WrapOpenSSLHeaders::WrapOpenSSLHeaders
    CODE
"#include <openssl/ssl.h>
#if defined(OPENSSL_NO_DTLS) || !defined(DTLS1_2_VERSION)
#  error OpenSSL without DTLS support
#endif

int main(void)
{
    /* BEGIN TEST: */
    /* END TEST: */
    return 0;
}
")

# ocsp
qt_config_compile_test(ocsp
    LABEL "OCSP stapling support in OpenSSL"
    LIBRARIES
        WrapOpenSSLHeaders::WrapOpenSSLHeaders
    CODE
"#include <openssl/ssl.h>
#include <openssl/ocsp.h>
#if defined(OPENSSL_NO_OCSP) || defined(OPENSSL_NO_TLSEXT)
#  error OpenSSL without OCSP stapling
#endif

int main(void)
{
    /* BEGIN TEST: */
    /* END TEST: */
    return 0;
}
")

# networklistmanager
qt_config_compile_test(networklistmanager
    LABEL "Network List Manager"
    CODE
"#include <netlistmgr.h>
#include <wrl/client.h>

int main(void)
{
    /* BEGIN TEST: */
using namespace Microsoft::WRL;
ComPtr<INetworkListManager> networkListManager;
ComPtr<IConnectionPoint> connectionPoint;
ComPtr<IConnectionPointContainer> connectionPointContainer;
networkListManager.As(&connectionPointContainer);
connectionPointContainer->FindConnectionPoint(IID_INetworkConnectionEvents, &connectionPoint);
    /* END TEST: */
    return 0;
}
"# FIXME: qmake: LIBS += -lOle32
)



#### Features

qt_feature("getifaddrs" PUBLIC
    LABEL "getifaddrs()"
    CONDITION TEST_getifaddrs
)
qt_feature_definition("getifaddrs" "QT_NO_GETIFADDRS" NEGATE VALUE "1")
qt_feature("ifr_index" PRIVATE
    LABEL "ifr_index"
    CONDITION TEST_ifr_index
)
qt_feature("ipv6ifname" PUBLIC
    LABEL "IPv6 ifname"
    CONDITION TEST_ipv6ifname
)
qt_feature_definition("ipv6ifname" "QT_NO_IPV6IFNAME" NEGATE VALUE "1")
qt_feature("libproxy" PRIVATE
    LABEL "libproxy"
    AUTODETECT OFF
    CONDITION Libproxy_FOUND
)
qt_feature("linux-netlink" PRIVATE
    LABEL "Linux AF_NETLINK"
    CONDITION LINUX AND NOT ANDROID AND TEST_linux_netlink
)
qt_feature("openssl" PRIVATE
    LABEL "OpenSSL"
    CONDITION QT_FEATURE_openssl_runtime OR QT_FEATURE_openssl_linked
    ENABLE false
)
qt_feature_definition("openssl" "QT_NO_OPENSSL" NEGATE)
qt_feature_config("openssl" QMAKE_PUBLIC_QT_CONFIG)
qt_feature("openssl-runtime"
    AUTODETECT NOT WASM
    CONDITION TEST_openssl_headers
    ENABLE INPUT_openssl STREQUAL 'yes' OR INPUT_openssl STREQUAL 'runtime'
    DISABLE INPUT_openssl STREQUAL 'no' OR INPUT_openssl STREQUAL 'linked' OR INPUT_ssl STREQUAL 'no'
)
qt_feature("openssl-linked" PRIVATE
    LABEL "  Qt directly linked to OpenSSL"
    AUTODETECT OFF
    CONDITION TEST_openssl
    ENABLE INPUT_openssl STREQUAL 'linked'
)
qt_feature_definition("openssl-linked" "QT_LINKED_OPENSSL")
qt_feature("securetransport" PUBLIC
    LABEL "SecureTransport"
    CONDITION APPLE
    DISABLE INPUT_ssl STREQUAL 'no'
)
qt_feature_definition("securetransport" "QT_SECURETRANSPORT")
qt_feature("schannel" PUBLIC
    LABEL "Schannel"
    CONDITION WIN32
    DISABLE INPUT_ssl STREQUAL 'no'
)
qt_feature_definition("schannel" "QT_SCHANNEL")
qt_feature("ssl" PUBLIC
    LABEL "SSL"
    CONDITION QT_FEATURE_securetransport OR QT_FEATURE_openssl OR QT_FEATURE_schannel
)
qt_feature_definition("ssl" "QT_NO_SSL" NEGATE VALUE "1")
qt_feature("dtls" PUBLIC
    SECTION "Networking"
    LABEL "DTLS"
    PURPOSE "Provides a DTLS implementation"
    CONDITION QT_FEATURE_openssl AND QT_FEATURE_udpsocket AND TEST_dtls
)
qt_feature("ocsp" PUBLIC
    SECTION "Networking"
    LABEL "OCSP-stapling"
    PURPOSE "Provides OCSP stapling support"
    CONDITION QT_FEATURE_opensslv11 AND TEST_ocsp
)
qt_feature("opensslv11" PUBLIC
    LABEL "OpenSSL 1.1"
    CONDITION QT_FEATURE_openssl
)
qt_feature("sctp" PUBLIC
    LABEL "SCTP"
    AUTODETECT OFF
    CONDITION TEST_sctp
)
qt_feature_definition("sctp" "QT_NO_SCTP" NEGATE VALUE "1")
qt_feature("system-proxies" PRIVATE
    LABEL "Use system proxies"
)
qt_feature("http" PUBLIC
    SECTION "Networking"
    LABEL "HTTP"
    PURPOSE "Provides support for the Hypertext Transfer Protocol in QNetworkAccessManager."
    CONDITION QT_FEATURE_thread
)
qt_feature_definition("http" "QT_NO_HTTP" NEGATE VALUE "1")
qt_feature("udpsocket" PUBLIC
    SECTION "Networking"
    LABEL "QUdpSocket"
    PURPOSE "Provides access to UDP sockets."
)
qt_feature_definition("udpsocket" "QT_NO_UDPSOCKET" NEGATE VALUE "1")
qt_feature("networkproxy" PUBLIC
    SECTION "Networking"
    LABEL "QNetworkProxy"
    PURPOSE "Provides network proxy support."
)
qt_feature_definition("networkproxy" "QT_NO_NETWORKPROXY" NEGATE VALUE "1")
qt_feature("socks5" PUBLIC
    SECTION "Networking"
    LABEL "SOCKS5"
    PURPOSE "Provides SOCKS5 support in QNetworkProxy."
    CONDITION QT_FEATURE_networkproxy
)
qt_feature_definition("socks5" "QT_NO_SOCKS5" NEGATE VALUE "1")
qt_feature("networkinterface" PUBLIC
    SECTION "Networking"
    LABEL "QNetworkInterface"
    PURPOSE "Supports enumerating a host's IP addresses and network interfaces."
    CONDITION NOT WASM
)
qt_feature_definition("networkinterface" "QT_NO_NETWORKINTERFACE" NEGATE VALUE "1")
qt_feature("networkdiskcache" PUBLIC
    SECTION "Networking"
    LABEL "QNetworkDiskCache"
    PURPOSE "Provides a disk cache for network resources."
    CONDITION QT_FEATURE_temporaryfile
)
qt_feature_definition("networkdiskcache" "QT_NO_NETWORKDISKCACHE" NEGATE VALUE "1")
qt_feature("brotli" PUBLIC
    SECTION "Networking"
    LABEL "Brotli Decompression Support"
    PURPOSE "Support for downloading and decompressing resources compressed with Brotli through QNetworkAccessManager."
    CONDITION WrapBrotli_FOUND
)
qt_feature_definition("brotli" "QT_NO_BROTLI" NEGATE VALUE "1")
qt_feature("localserver" PUBLIC
    SECTION "Networking"
    LABEL "QLocalServer"
    PURPOSE "Provides a local socket based server."
    CONDITION QT_FEATURE_temporaryfile
)
qt_feature_definition("localserver" "QT_NO_LOCALSERVER" NEGATE VALUE "1")
qt_feature("dnslookup" PUBLIC
    SECTION "Networking"
    LABEL "QDnsLookup"
    PURPOSE "Provides API for DNS lookups."
    CONDITION NOT INTEGRITY
)
qt_feature("gssapi" PUBLIC
    SECTION "Networking"
    LABEL "GSSAPI"
    PURPOSE "Enable SPNEGO authentication through GSSAPI"
    CONDITION NOT WIN32 AND GSSAPI_FOUND
)
qt_feature_definition("gssapi" "QT_NO_GSSAPI" NEGATE VALUE "1")
qt_feature("sspi" PUBLIC
    SECTION "Networking"
    LABEL "SSPI"
    PURPOSE "Enable NTLM/SPNEGO authentication through SSPI"
    CONDITION WIN32
)
qt_feature_definition("sspi" "QT_NO_SSPI" NEGATE VALUE "1")
qt_feature("networklistmanager" PRIVATE
    SECTION "Networking"
    LABEL "Network List Manager"
    PURPOSE "Use Network List Manager to keep track of network connectivity"
    CONDITION WIN32 AND TEST_networklistmanager
)
qt_feature("topleveldomain" PUBLIC
    SECTION "Networking"
    LABEL "qTopLevelDomain()"
    PURPOSE "Provides support for extracting the top level domain from URLs.  If enabled, a binary dump of the Public Suffix List (http://www.publicsuffix.org, Mozilla License) is included. The data is then also used in QNetworkCookieJar::validateCookie."
)
qt_configure_add_summary_section(NAME "Qt Network")
qt_configure_add_summary_entry(ARGS "getifaddrs")
qt_configure_add_summary_entry(ARGS "ipv6ifname")
qt_configure_add_summary_entry(ARGS "libproxy")
qt_configure_add_summary_entry(
    ARGS "linux-netlink"
    CONDITION LINUX
)
qt_configure_add_summary_entry(
    ARGS "securetransport"
    CONDITION APPLE
)
qt_configure_add_summary_entry(
    ARGS "schannel"
    CONDITION WIN32
)
qt_configure_add_summary_entry(ARGS "openssl")
qt_configure_add_summary_entry(ARGS "openssl-linked")
qt_configure_add_summary_entry(ARGS "opensslv11")
qt_configure_add_summary_entry(ARGS "dtls")
qt_configure_add_summary_entry(ARGS "ocsp")
qt_configure_add_summary_entry(ARGS "sctp")
qt_configure_add_summary_entry(ARGS "system-proxies")
qt_configure_add_summary_entry(ARGS "gssapi")
qt_configure_add_summary_entry(ARGS "brotli")
qt_configure_end_summary_section() # end of "Qt Network" section
# special case begin
qt_configure_add_report_entry(
    TYPE NOTE
    MESSAGE "When linking against OpenSSL, you can override the default library names through OPENSSL_LIBS. For example: OPENSSL_LIBS='-L/opt/ssl/lib -lssl -lcrypto' ./configure -openssl-linked"
    CONDITION NOT ANDROID AND QT_FEATURE_openssl_linked AND ( NOT TEST_openssl.source EQUAL 0 ) AND OPENSSL_ROOT_DIR STREQUAL '' AND INPUT_openssl.libs STREQUAL '' AND INPUT_openssl.libs.debug STREQUAL '' OR FIXME
)
# special case end
