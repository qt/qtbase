TEMPLATE=subdirs
SUBDIRS=\
   animation \
   codecs \
   concurrent \
   global \
   io \
   itemmodels \
   json \
   kernel \
   plugin \
   statemachine \
   thread \
   tools \
   xml

!contains(QT_CONFIG, concurrent): SUBDIRS -= concurrent
