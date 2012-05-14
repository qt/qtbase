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

!contains(QT_CONFIG, private_tests): SUBDIRS -= \
           qpixmapcache \

