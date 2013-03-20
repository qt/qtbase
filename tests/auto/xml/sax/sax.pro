TEMPLATE=subdirs
SUBDIRS=\
   qxml \

qtHaveModule(network): SUBDIRS += \
   qxmlinputsource \
   qxmlsimplereader \

