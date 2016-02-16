TEMPLATE=subdirs

SUBDIRS = \
   kernel

!uikit: SUBDIRS += \
   animation \
   codecs \
   global \
   io \
   itemmodels \
   json \
   mimetypes \
   plugin \
   statemachine \
   thread \
   tools \
   xml
