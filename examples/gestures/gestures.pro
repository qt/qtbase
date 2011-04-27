TEMPLATE      = \
              subdirs
SUBDIRS       = \
              imagegestures

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/gestures
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS gestures.pro README
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/gestures
INSTALLS += target sources
