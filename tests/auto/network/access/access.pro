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
   hpack \
   http2

!contains(QT_CONFIG, private_tests): SUBDIRS -= \
          qhttpnetworkconnection \
          qhttpnetworkreply \
          qftp \
          hpack \
          http2
