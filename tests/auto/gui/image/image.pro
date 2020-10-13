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
   qiconhighdpi

!qtHaveModule(network): SUBDIRS -= \
    qimagereader

!qtConfig(private_tests): SUBDIRS -= \
           qpixmapcache \

# QTBUG-87669
android: SUBDIRS -= \
            qimagereader \
            qicon
