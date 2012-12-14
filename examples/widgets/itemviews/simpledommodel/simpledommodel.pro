HEADERS     = domitem.h \
              dommodel.h \
              mainwindow.h
SOURCES     = domitem.cpp \
              dommodel.cpp \
              main.cpp \
              mainwindow.cpp
QT      += xml widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/itemviews/simpledommodel
INSTALLS += target

