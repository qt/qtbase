TEMPLATE      = subdirs
SUBDIRS       = \
              chip \
              elasticnodes \
              embeddeddialogs \
              collidingmice \
              basicgraphicslayouts \
              diagramscene \
              dragdroprobot \
              flowlayout \
              anchorlayout \
              simpleanchorlayout \
              weatheranchorlayout

contains(DEFINES, QT_NO_CURSOR)|!qtConfig(draganddrop): SUBDIRS -= dragdroprobot
