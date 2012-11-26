HEADERS     = treeitem.h \
              treemodel.h
RESOURCES   = simpletreemodel.qrc
SOURCES     = treeitem.cpp \
              treemodel.cpp \
              main.cpp
CONFIG  += qt

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/itemviews/simpletreemodel
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro *.txt
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/itemviews/simpletreemodel
INSTALLS += target sources

QT += widgets
