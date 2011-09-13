CONFIG += qttest_p4
QT += widgets

SOURCES		+= tst_qdesktopservices.cpp
TARGET		= tst_qdesktopservices
symbian: {
   dummy.files = text\\testfile.txt
   dummy.path = .

   text.files = text\\*
   text.path = \\data\\others

   image.files = image\\*
   image.path = \\data\\images

   audio.files = audio\\*
   audio.path = \\data\\sounds

   video.files = video\\*
   video.path = \\data\\videos

   install.files = install\\*
   install.path = \\data\\installs

   DEPLOYMENT += image audio video install

   # These are only needed for manual tests
   #DEPLOYMENT += dummy text
   }

