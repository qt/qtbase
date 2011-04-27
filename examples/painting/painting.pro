TEMPLATE      = subdirs
SUBDIRS       = basicdrawing \
                concentriccircles \
                imagecomposition \
                painterpaths \
                transformations

!wince*:!symbian: SUBDIRS += fontsampler

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/painting
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS painting.pro README
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/painting
INSTALLS += target sources

symbian: CONFIG += qt_example
