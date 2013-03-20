TEMPLATE=subdirs
SUBDIRS=\
   qimagereader \
   qicoimageformat \
   qpixmap \
   qpixmapcache \
   qimage \
   qimageiohandler \
   qimagewriter \
   qmovie \
   qpicture \
   qicon \

!qtHaveModule(network): SUBDIRS -= \
    qimagereader

!contains(QT_CONFIG, private_tests): SUBDIRS -= \
           qpixmapcache \

