HEADERS       = randomlistmodel.h
SOURCES       = randomlistmodel.cpp \
                main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/contiguouscache
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS contiguouscache.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/contiguouscache
INSTALLS += target sources
