SOURCES += main.cpp
FORMS += dials.ui

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/touch/dials
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS dials.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/touch/dials
INSTALLS += target sources
