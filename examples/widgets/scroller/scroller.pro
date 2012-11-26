TEMPLATE = subdirs
SUBDIRS +=  graphicsview

# install
sources.files = scroller.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/scroller
INSTALLS += sources
