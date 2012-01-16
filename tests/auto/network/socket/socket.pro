TEMPLATE=subdirs
SUBDIRS=\
   qhttpsocketengine \
   qudpsocket \
   qtcpsocket \
   #qlocalsocket \  # FIXME: uses qtscript (QTBUG-19242)
   qtcpserver \
   qsocks5socketengine \
   qabstractsocket \
   platformsocketengine \

!contains(QT_CONFIG, private_tests): SUBDIRS -= \
   platformsocketengine \
   qtcpsocket \
   qhttpsocketengine \
   qsocks5socketengine \
