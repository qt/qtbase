TEMPLATE=subdirs

SUBDIRS = \
    qmimetype \
    qmimedatabase

!qtConfig(private_tests): SUBDIRS -= \
    qmimetype
