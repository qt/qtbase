TEMPLATE=subdirs
SUBDIRS=\
   qdnslookup \
   qdnslookup_appless \
   qhostinfo \
#   qnetworkproxyfactory \ # Uses a hardcoded proxy configuration
   qauthenticator \
   qnetworkproxy \
   qnetworkinterface \
   qnetworkaddressentry \
   qhostaddress \

!contains(QT_CONFIG, private_tests): SUBDIRS -= \
    qauthenticator \
    qhostinfo \

