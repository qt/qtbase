TEMPLATE = subdirs

SUBDIRS = part1 \
          part2 \
          part3 \
          part4 \
          part5

# install
sources.files = *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/tutorials/gettingStarted/gsQt
INSTALLS += sources

QT += widgets
