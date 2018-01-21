QT += widgets
requires(qtConfig(combobox))

HEADERS       = norwegianwoodstyle.h \
                widgetgallery.h
SOURCES       = main.cpp \
                norwegianwoodstyle.cpp \
                widgetgallery.cpp
RESOURCES     = styles.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/widgets/styles
INSTALLS += target
