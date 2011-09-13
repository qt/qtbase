TEMPLATE=subdirs
SUBDIRS=\
   qimagereader \
   qicoimageformat \
   qpixmap \
   qpixmapcache \
   qimage \
   qpixmapfilter \
   qimageiohandler \
   qimagewriter \
   qmovie \
   qvolatileimage \
   qicon \
   qpicture \

!contains(QT_CONFIG, private_tests): SUBDIRS -= \
           qpixmapcache \

