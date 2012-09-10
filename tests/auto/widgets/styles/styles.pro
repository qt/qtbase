TEMPLATE=subdirs
SUBDIRS=\
   qmacstyle \
   qstyle \
   qstyleoption \
   qstylesheetstyle \

!contains(QT_CONFIG, private_tests): SUBDIRS -= \
           qstylesheetstyle \

# This test can only be run on Mac OS:
!mac:SUBDIRS -= \
    qmacstyle \

!contains( styles, motif ):SUBDIRS -= \
    qstylesheetstyle \
