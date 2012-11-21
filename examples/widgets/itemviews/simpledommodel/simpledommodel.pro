HEADERS     = domitem.h \
              dommodel.h \
              mainwindow.h
SOURCES     = domitem.cpp \
              dommodel.cpp \
              main.cpp \
              mainwindow.cpp
CONFIG  += qt
QT      += xml widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/itemviews/simpledommodel
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/itemviews/simpledommodel
INSTALLS += target sources

