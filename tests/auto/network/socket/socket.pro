TEMPLATE=subdirs
QT_FOR_CONFIG += network

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

!qtConfig(private_tests): SUBDIRS -= \
   platformsocketengine \
   qtcpsocket \
   qhttpsocketengine \
   qsocks5socketengine \

!qtConfig(sctp): SUBDIRS -= \
   qsctpsocket \

winrt: SUBDIRS -= \
   qhttpsocketengine \
   qsocks5socketengine \
