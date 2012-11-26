HEADERS       = norwegianwoodstyle.h \
                widgetgallery.h
SOURCES       = main.cpp \
                norwegianwoodstyle.cpp \
                widgetgallery.cpp
RESOURCES     = styles.qrc

REQUIRES += "contains(styles, windows)"

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/widgets/styles
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS styles.pro images
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/widgets/styles
INSTALLS += target sources

QT += widgets

simulator: warning(This example might not fully work on Simulator platform)
