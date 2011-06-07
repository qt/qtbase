HEADERS       = randomlistmodel.h
SOURCES       = randomlistmodel.cpp \
                main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/contiguouscache
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS contiguouscache.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/contiguouscache
INSTALLS += target sources

symbian: CONFIG += qt_example
maemo5: CONFIG += qt_example

symbian: warning(This example might not fully work on Symbian platform)
maemo5: warning(This example might not fully work on Maemo platform)
simulator: warning(This example might not fully work on Simulator platform)
