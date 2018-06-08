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

contains(DEFINES, QT_NO_CURSOR)|!qtConfig(draganddrop): SUBDIRS -= dragdroprobot

qtHaveModule(opengl):!qtConfig(opengles.):!qtConfig(dynamicgl) {
    SUBDIRS += boxes
}
