TEMPLATE = subdirs
SUBDIRS = pinchzoom fingerpaint knobs dials

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/touch
sources.files = touch.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/touch
INSTALLS += target sources
