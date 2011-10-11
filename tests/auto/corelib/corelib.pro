TEMPLATE=subdirs
SUBDIRS=\
   animation \
   codecs \
   concurrent \
   global \
   io \
   kernel \
   plugin \
   statemachine \
   thread \
   tools \
   xml

!contains(QT_CONFIG, concurrent): SUBDIRS -= concurrent
