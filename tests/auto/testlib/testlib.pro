TEMPLATE=subdirs
SUBDIRS=\
   qsignalspy \
   selftests \

qtHaveModule(widgets):SUBDIRS += qabstractitemmodeltester
