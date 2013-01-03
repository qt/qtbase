QT += widgets

HEADERS       = shapeitem.h \
                sortingbox.h
SOURCES       = main.cpp \
                shapeitem.cpp \
                sortingbox.cpp
RESOURCES     = tooltips.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/widgets/tooltips
INSTALLS += target
