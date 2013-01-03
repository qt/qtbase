TEMPLATE      = subdirs
SUBDIRS       = \
              chip \
              elasticnodes \
              embeddeddialogs \
              collidingmice \
              padnavigator \
              basicgraphicslayouts \
              diagramscene \
              dragdroprobot \
              flowlayout \
              anchorlayout \
              simpleanchorlayout \
              weatheranchorlayout

contains(DEFINES, QT_NO_CURSOR)|contains(DEFINES, QT_NO_DRAGANDDROP): SUBDIRS -= dragdroprobot

qtHaveModule(opengl):!contains(QT_CONFIG, opengles.) {
    SUBDIRS += boxes
}
