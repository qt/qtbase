TARGET = mv_formatting

TEMPLATE = app

SOURCES += main.cpp \
           mymodel.cpp

HEADERS += mymodel.h

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/tutorials/modelview/2_formatting
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS 2_formatting.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/tutorials/modelview/2_formatting
INSTALLS += target sources

QT += widgets

simulator: warning(This example might not fully work on Simulator platform)
