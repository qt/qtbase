TEMPLATE=subdirs
SUBDIRS=\
   qhttpsocketengine \
   qudpsocket \
   qtcpsocket \
   qlocalsocket \
   qtcpserver \
   qsocks5socketengine \
   qabstractsocket \
   platformsocketengine \

!contains(QT_CONFIG, private_tests): SUBDIRS -= \
   platformsocketengine \
   qtcpsocket \
   qhttpsocketengine \
   qsocks5socketengine \

winrt: SUBDIRS -= \
   qhttpsocketengine \
   qsocks5socketengine \
