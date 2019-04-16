TEMPLATE = subdirs
SUBDIRS = \
   outformat \
   qsignalspy \
   selftests \

qtHaveModule(widgets): SUBDIRS += qabstractitemmodeltester
