TEMPLATE      = subdirs
SUBDIRS       = basicdrawing \
                concentriccircles \
                imagecomposition \
                painterpaths \
                transformations

!wince*:!symbian: SUBDIRS += fontsampler

contains(QT_CONFIG, svg): SUBDIRS += svggenerator svgviewer

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/painting
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS painting.pro README
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/painting
INSTALLS += target sources

symbian: CONFIG += qt_example
