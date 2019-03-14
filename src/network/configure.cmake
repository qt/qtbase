

#### Inputs



#### Libraries

find_package(Libproxy)
set_package_properties(Libproxy PROPERTIES TYPE OPTIONAL)
find_package(OpenSSL)
set_package_properties(OpenSSL PROPERTIES TYPE OPTIONAL)


#### Tests

# getifaddrs
qt_config_compile_test(getifaddrs
    LABEL "getifaddrs()"
"
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <ifaddrs.h>

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    /* BEGIN TEST: */
ifaddrs *list;
getifaddrs(&list);
freeifaddrs(list);
    /* END TEST: */
    return 0;
}
"# FIXME: use: network
)

# ipv6ifname
qt_config_compile_test(ipv6ifname
    LABEL "IPv6 ifname"
"
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    /* BEGIN TEST: */
char buf[IFNAMSIZ];
if_nametoindex(\"eth0\");
if_indextoname(1, buf);
if_freenameindex(if_nameindex());
    /* END TEST: */
    return 0;
}
"# FIXME: use: network
)

# linux-netlink
qt_config_compile_test(linux_netlink
    LABEL "Linux AF_NETLINK sockets"
"
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
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
"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    /* BEGIN TEST: */
sctp_initmsg sctpInitMsg;
socklen_t sctpInitMsgSize = sizeof(sctpInitMsg);
(void) socket(PF_INET, SOCK_STREAM, IPPROTO_SCTP);
(void) getsockopt(-1, SOL_SCTP, SCTP_INITMSG, &sctpInitMsg, &sctpInitMsgSize);
    /* END TEST: */
    return 0;
}
"# FIXME: use: network
)

# dtls
qt_config_compile_test(dtls
    LABEL "DTLS support in OpenSSL"
"
#include <openssl/ssl.h>
#if defined(OPENSSL_NO_DTLS) || !defined(DTLS1_2_VERSION)
#  error OpenSSL without DTLS support
#endif
int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    /* BEGIN TEST: */

    /* END TEST: */
    return 0;
}
"# FIXME: use: openssl
)



#### Features

qt_feature("corewlan" PUBLIC PRIVATE
    LABEL "CoreWLan"
    CONDITION libs.corewlan OR FIXME
    EMIT_IF APPLE
)
qt_feature_definition("corewlan" "QT_NO_COREWLAN" NEGATE VALUE "1")
qt_feature("getifaddrs" PUBLIC
    LABEL "getifaddrs()"
    CONDITION TEST_getifaddrs
)
qt_feature_definition("getifaddrs" "QT_NO_GETIFADDRS" NEGATE VALUE "1")
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
qt_feature("linux_netlink" PRIVATE
    LABEL "Linux AF_NETLINK"
    CONDITION LINUX AND TEST_linux_netlink
)
qt_feature("openssl" PRIVATE
    LABEL "OpenSSL"
    AUTODETECT NOT WINRT AND NOT WASM
    CONDITION NOT QT_FEATURE_securetransport AND ( QT_FEATURE_openssl_linked OR OPENSSL_INCLUDE_DIR )
    ENABLE INPUT_openssl STREQUAL 'yes' OR INPUT_openssl STREQUAL 'linked' OR INPUT_openssl STREQUAL 'runtime'
    DISABLE INPUT_openssl STREQUAL 'no' OR INPUT_ssl STREQUAL 'no'
)
qt_feature_definition("openssl" "QT_NO_OPENSSL" NEGATE)
qt_feature("openssl_linked" PRIVATE
    LABEL "  Qt directly linked to OpenSSL"
    CONDITION NOT QT_FEATURE_securetransport AND OpenSSL_FOUND
    ENABLE INPUT_openssl STREQUAL 'linked'
    DISABLE ( NOT INPUT_openssl STREQUAL 'linked' )
)
qt_feature_definition("openssl_linked" "QT_LINKED_OPENSSL")
qt_feature("securetransport" PRIVATE
    LABEL "SecureTransport"
    CONDITION APPLE AND ( INPUT_openssl STREQUAL '' OR INPUT_openssl STREQUAL 'no' )
    DISABLE INPUT_securetransport STREQUAL 'no' OR INPUT_ssl STREQUAL 'no'
)
qt_feature_definition("securetransport" "QT_SECURETRANSPORT")
qt_feature("ssl" PUBLIC
    LABEL "SSL"
    CONDITION WINRT OR QT_FEATURE_securetransport OR QT_FEATURE_openssl
)
qt_feature_definition("ssl" "QT_NO_SSL" NEGATE VALUE "1")
qt_feature("dtls" PUBLIC
    SECTION "Networking"
    LABEL "DTLS"
    PURPOSE "Provides a DTLS implementation"
    CONDITION QT_FEATURE_openssl AND TEST_dtls
)
qt_feature("opensslv11" PUBLIC
    LABEL "OpenSSL 1.1"
    CONDITION QT_FEATURE_openssl AND ( OPENSSL_VERSION VERSION_GREATER_EQUAL "1.1.0" )
)
qt_feature("sctp" PUBLIC
    LABEL "SCTP"
    AUTODETECT OFF
    CONDITION TEST_sctp
)
qt_feature_definition("sctp" "QT_NO_SCTP" NEGATE VALUE "1")
qt_feature("system_proxies" PRIVATE
    LABEL "Use system proxies"
)
qt_feature("ftp" PUBLIC
    SECTION "Networking"
    LABEL "FTP"
    PURPOSE "Provides support for the File Transfer Protocol in QNetworkAccessManager."
    CONDITION QT_FEATURE_textdate
)
qt_feature_definition("ftp" "QT_NO_FTP" NEGATE VALUE "1")
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
qt_feature("bearermanagement" PUBLIC
    SECTION "Networking"
    LABEL "Bearer management"
    PURPOSE "Provides bearer management for the network stack."
    CONDITION QT_FEATURE_thread AND QT_FEATURE_library AND QT_FEATURE_networkinterface AND QT_FEATURE_properties
)
qt_feature_definition("bearermanagement" "QT_NO_BEARERMANAGEMENT" NEGATE VALUE "1")
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
)
