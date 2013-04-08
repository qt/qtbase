QT += widgets

HEADERS     = mysortfilterproxymodel.h \
              window.h \
              filterwidget.h
SOURCES     = main.cpp \
              mysortfilterproxymodel.cpp \
              window.cpp \
              filterwidget.cpp

RESOURCES +=  customsortfiltermodel.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/itemviews/customsortfiltermodel
INSTALLS += target
