TEMPLATE=subdirs
SUBDIRS=\
   qdnslookup \
   qdnslookup_appless \
   qhostinfo \
   qnetworkproxyfactory \
   qauthenticator \
   qnetworkproxy \
   qnetworkinterface \
   qnetworkdatagram \
   qnetworkaddressentry \
   qhostaddress \

osx: SUBDIRS -= \ # QTBUG-41847
    qhostinfo \

!qtConfig(private_tests): SUBDIRS -= \
    qauthenticator \
    qhostinfo \

