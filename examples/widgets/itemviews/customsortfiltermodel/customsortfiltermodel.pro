HEADERS     = mysortfilterproxymodel.h \
              window.h
SOURCES     = main.cpp \
              mysortfilterproxymodel.cpp \
              window.cpp
CONFIG     += qt

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/itemviews/customsortfiltermodel
INSTALLS += target

QT += widgets
