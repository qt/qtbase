FORMS       = mainwindow.ui
HEADERS     = mainwindow.h \
              treeitem.h \
              treemodel.h
RESOURCES   = editabletreemodel.qrc
SOURCES     = mainwindow.cpp \
              treeitem.cpp \
              treemodel.cpp \
              main.cpp
CONFIG  += qt

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/itemviews/editabletreemodel
sources.files = $$FORMS $$HEADERS $$RESOURCES $$SOURCES *.pro *.txt
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/itemviews/editabletreemodel
INSTALLS += target sources

QT += widgets
