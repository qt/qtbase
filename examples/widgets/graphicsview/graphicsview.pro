TEMPLATE      = subdirs
SUBDIRS       = \
              chip \
              elasticnodes \
              embeddeddialogs \
              collidingmice \
              basicgraphicslayouts \
              diagramscene \
              dragdroprobot \
              simpleanchorlayout

contains(DEFINES, QT_NO_CURSOR)|!qtConfig(draganddrop): SUBDIRS -= dragdroprobot
