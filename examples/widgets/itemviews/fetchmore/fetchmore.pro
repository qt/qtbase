QT += widgets
requires(qtConfig(listview))

HEADERS   = filelistmodel.h \
            window.h
SOURCES   = filelistmodel.cpp \
            main.cpp \
            window.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/itemviews/fetchmore
INSTALLS += target
