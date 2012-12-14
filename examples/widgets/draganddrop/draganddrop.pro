TEMPLATE    = subdirs
SUBDIRS     = draggableicons \
              draggabletext \
              dropsite \
              fridgemagnets \
              puzzle

wince*: SUBDIRS -= dropsite
