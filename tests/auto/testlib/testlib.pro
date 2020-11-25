TEMPLATE = subdirs
SUBDIRS = \
   initmain \
   outformat \
   qsignalspy \
   selftests \

# QTBUG-88507
android: SUBDIRS -= \
    selftests

qtHaveModule(widgets): SUBDIRS += qabstractitemmodeltester
