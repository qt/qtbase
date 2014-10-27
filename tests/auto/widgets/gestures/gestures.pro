TEMPLATE=subdirs
SUBDIRS=\
   qgesturerecognizer \

mac: SUBDIRS -= \ # Uses native recognizers
   qgesturerecognizer \
