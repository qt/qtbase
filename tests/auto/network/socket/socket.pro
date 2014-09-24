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
   qsctpsocket \

!contains(QT_CONFIG, private_tests): SUBDIRS -= \
   platformsocketengine \
   qtcpsocket \
   qhttpsocketengine \
   qsocks5socketengine \

!contains(QT_CONFIG, sctp): SUBDIRS -= \
   qsctpsocket \

winrt: SUBDIRS -= \
   qhttpsocketengine \
   qsocks5socketengine \
