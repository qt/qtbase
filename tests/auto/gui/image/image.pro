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

!qtConfig(private_tests): SUBDIRS -= \
           qpixmapcache \

