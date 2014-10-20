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

osx: SUBDIRS -= \ # QTBUG-41847
    qhostinfo \

!contains(QT_CONFIG, private_tests): SUBDIRS -= \
    qauthenticator \
    qhostinfo \

