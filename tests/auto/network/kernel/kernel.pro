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
   qhostaddress

# QTBUG-41847
osx: SUBDIRS -= qhostinfo

!qtConfig(private_tests): SUBDIRS -= \
    qauthenticator \
    qhostinfo

