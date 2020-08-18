TEMPLATE=subdirs

SUBDIRS = \
   kernel

!uikit: SUBDIRS += \
   animation \
   global \
   io \
   itemmodels \
   mimetypes \
   plugin \
   serialization \
   text \
   thread \
   time \
   tools
