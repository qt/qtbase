HEADERS       = norwegianwoodstyle.h \
                widgetgallery.h
SOURCES       = main.cpp \
                norwegianwoodstyle.cpp \
                widgetgallery.cpp
RESOURCES     = styles.qrc

REQUIRES += "contains(styles, motif)"

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/styles
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS styles.pro images
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/styles
INSTALLS += target sources

symbian: CONFIG += qt_example
QT += widgets
