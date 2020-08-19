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

!qtConfig(private_tests): SUBDIRS -= \
          qsidebar \

mac:qinputdialog.CONFIG += no_check_target # QTBUG-25496

