TEMPLATE=subdirs
QT_FOR_CONFIG += network

SUBDIRS=\
   qnetworkdiskcache \
   qnetworkcookiejar \
   qnetworkaccessmanager \
   qnetworkcookie \
   qnetworkrequest \
   qhttpnetworkconnection \
   qnetworkreply \
   qnetworkcachemetadata \
   qhttpnetworkreply \
   qabstractnetworkcache \
   hpack \
   http2 \
   hsts \
   qdecompresshelper

!qtConfig(private_tests): SUBDIRS -= \
          qhttpnetworkconnection \
          qhttpnetworkreply \
          hpack \
          http2 \
          hsts \
          qdecompresshelper
