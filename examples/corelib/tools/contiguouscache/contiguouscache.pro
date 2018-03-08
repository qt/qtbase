QT += widgets
requires(qtConfig(listview))

HEADERS       = randomlistmodel.h
SOURCES       = randomlistmodel.cpp \
                main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/corelib/tools/contiguouscache
INSTALLS += target
