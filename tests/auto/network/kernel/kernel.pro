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

winrt: SUBDIRS -= \
   qnetworkproxy \
   qnetworkproxyfactory \

!contains(QT_CONFIG, private_tests): SUBDIRS -= \
    qauthenticator \
    qhostinfo \

