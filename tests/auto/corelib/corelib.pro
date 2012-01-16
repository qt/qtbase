TEMPLATE=subdirs
SUBDIRS=\
   animation \
   codecs \
   concurrent \
   global \
   io \
   itemmodels \
   kernel \
   plugin \
   statemachine \
   thread \
   tools \
   xml

!contains(QT_CONFIG, concurrent): SUBDIRS -= concurrent
