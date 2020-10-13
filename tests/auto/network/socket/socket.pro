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

android: SUBDIRS -= \
    # QTBUG-87387
    qlocalsocket \
    # QTBUG-87388
    qtcpserver

!qtConfig(private_tests): SUBDIRS -= \
   platformsocketengine \
   qtcpsocket \
   qhttpsocketengine \
   qsocks5socketengine \

!qtConfig(sctp): SUBDIRS -= \
   qsctpsocket \
