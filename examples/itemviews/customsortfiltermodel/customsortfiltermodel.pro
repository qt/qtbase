HEADERS     = mysortfilterproxymodel.h \
              window.h
SOURCES     = main.cpp \
              mysortfilterproxymodel.cpp \
              window.cpp
CONFIG     += qt

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/itemviews/customsortfiltermodel
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/itemviews/customsortfiltermodel
INSTALLS += target sources

QT += widgets
