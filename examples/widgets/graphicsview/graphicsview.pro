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

contains(QT_CONFIG, opengl):!contains(QT_CONFIG, opengles1):!contains(QT_CONFIG, opengles2):{
    SUBDIRS += boxes
}

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/graphicsview
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS graphicsview.pro README
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/graphicsview
INSTALLS += target sources

QT += widgets
