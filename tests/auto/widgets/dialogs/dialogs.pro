TEMPLATE=subdirs
SUBDIRS=\
   qabstractprintdialog \
   qcolordialog \
   qdialog \
   qerrormessage \
   qfiledialog \
   qfiledialog2 \
   qfilesystemmodel \
   qfontdialog \
   qinputdialog \
   qmessagebox \
   qprogressdialog \
   qsidebar \
   qwizard \

!contains(QT_CONFIG, private_tests): SUBDIRS -= \
          qsidebar \
