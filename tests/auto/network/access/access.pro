TEMPLATE=subdirs
SUBDIRS=\
   qnetworkdiskcache \
   qnetworkcookiejar \
   qnetworkaccessmanager \
   qnetworkcookie \
   qnetworkrequest \
   qhttpnetworkconnection \
   qnetworkreply \
   spdy \
   qnetworkcachemetadata \
   qftp \
   qhttpnetworkreply \
   qabstractnetworkcache \
   hpack

!contains(QT_CONFIG, private_tests): SUBDIRS -= \
          qhttpnetworkconnection \
          qhttpnetworkreply \
          qftp \
          hpack

contains(QT_CONFIG, openssl) | contains(QT_CONFIG, openssl-linked) {
    contains(QT_CONFIG, private_tests) {
        SUBDIRS += \
            http2
    }
}
