TEMPLATE=subdirs
SUBDIRS=\
   qmacstyle \
   qstyle \
   qstyleoption \
   qstylesheetstyle \

!qtConfig(private_tests): SUBDIRS -= \
           qstylesheetstyle \

