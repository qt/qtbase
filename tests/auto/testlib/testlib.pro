TEMPLATE = subdirs
SUBDIRS = \
   initmain \
   outformat \
   qsignalspy \
   selftests \

qtHaveModule(widgets): SUBDIRS += qabstractitemmodeltester
