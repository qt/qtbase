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

wince*|!qtHaveModule(printsupport):SUBDIRS -= qabstractprintdialog

!contains(QT_CONFIG, private_tests): SUBDIRS -= \
          qsidebar \

mac:qinputdialog.CONFIG += no_check_target # QTBUG-25496
win32-g++*: SUBDIRS -= qfilesystemmodel # QTBUG-29403
