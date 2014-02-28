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

!contains(QT_CONFIG, private_tests): SUBDIRS -= \
          qhttpnetworkconnection \
          qhttpnetworkreply \
          qftp \

