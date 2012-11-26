TEMPLATE      = \
              subdirs
SUBDIRS       = \
              imagegestures

# install
target.path = $$[QT_INSTALL_EXAMPLES]/gestures
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS gestures.pro README
sources.path = $$[QT_INSTALL_EXAMPLES]/gestures
INSTALLS += target sources
QT += widgets
