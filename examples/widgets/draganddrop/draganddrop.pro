TEMPLATE    = subdirs
SUBDIRS     = draggableicons \
              draggabletext \
              dropsite \
              fridgemagnets \
              puzzle

wince*: SUBDIRS -= dropsite
# install
sources.files = README *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/draganddrop
INSTALLS += sources
QT += widgets
