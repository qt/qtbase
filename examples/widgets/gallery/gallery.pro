QT += widgets
requires(qtConfig(combobox))

HEADERS       = widgetgallery.h
SOURCES       = main.cpp \
                widgetgallery.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/gallery
INSTALLS += target
