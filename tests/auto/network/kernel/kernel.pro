TEMPLATE=subdirs
SUBDIRS=\
   qdnslookup \
   qdnslookup_appless \
   qhostinfo \
   qnetworkproxyfactory \
   qauthenticator \
   qnetworkproxy \
   qnetworkinterface \
   qnetworkaddressentry \
   qhostaddress \

!contains(QT_CONFIG, private_tests): SUBDIRS -= \
    qauthenticator \
    qhostinfo \

