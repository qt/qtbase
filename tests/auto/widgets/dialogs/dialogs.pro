TEMPLATE=subdirs
SUBDIRS=\
   qcolordialog \
   qdialog \
   qerrormessage \
   qfiledialog \
   qfiledialog2 \
   qfontdialog \
   qinputdialog \
   qmessagebox \
   qprogressdialog \
   qsidebar \
   qwizard \

# QTBUG-87671
android: SUBDIRS -= \
    qfiledialog \
    qmessagebox

!qtConfig(private_tests): SUBDIRS -= \
          qsidebar \

mac:qinputdialog.CONFIG += no_check_target # QTBUG-25496

