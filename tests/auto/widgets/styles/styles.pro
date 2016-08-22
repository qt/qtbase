TEMPLATE=subdirs
SUBDIRS=\
   qmacstyle \
   qstyle \
   qstyleoption \
   qstylesheetstyle \

!qtConfig(private_tests): SUBDIRS -= \
           qstylesheetstyle \

# This test can only be run on Mac OS:
!mac:SUBDIRS -= \
    qmacstyle \

uikit|android|qnx: SUBDIRS -= \
    qstylesheetstyle \
