TEMPLATE=subdirs

SUBDIRS = \
    qmimetype \
    qmimedatabase

!contains(QT_CONFIG, private_tests): SUBDIRS -= \
    qmimetype
