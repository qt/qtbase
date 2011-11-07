TEMPLATE=subdirs
SUBDIRS=\
# disabled in src/widgets/styles/styles.pri, so disable the test as well
#   qmacstyle \
   qstyle \
   qstyleoption \
   qstylesheetstyle \

!contains(QT_CONFIG, private_tests): SUBDIRS -= \
           qstylesheetstyle \

# This test can only be run on Mac OS:
!mac:SUBDIRS -= \
    qmacstyle \
