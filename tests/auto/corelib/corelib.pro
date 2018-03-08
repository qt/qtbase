TEMPLATE=subdirs

SUBDIRS = \
   kernel

!uikit: SUBDIRS += \
   animation \
   codecs \
   global \
   io \
   itemmodels \
   mimetypes \
   plugin \
   serialization \
   statemachine \
   thread \
   tools
