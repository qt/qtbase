TEMPLATE = subdirs
SUBDIRS += dir_gsqt
QT += widgets

dir_gsqt.file = gsQt/gsqt.pro

# install
sources.files = gettingStarted.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/tutorials/gettingStarted
INSTALLS += sources
