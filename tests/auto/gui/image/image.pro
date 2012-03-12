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

!contains(QT_CONFIG, private_tests): SUBDIRS -= \
           qpixmapcache \

