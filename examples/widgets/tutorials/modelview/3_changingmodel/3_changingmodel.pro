TARGET = mv_changingmodel

TEMPLATE = app

SOURCES += main.cpp \
           mymodel.cpp

HEADERS += mymodel.h

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/tutorials/modelview/3_changingmodel
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS 3_changingmodel.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/tutorials/modelview/3_changingmodel
INSTALLS += target sources

QT += widgets

simulator: warning(This example might not fully work on Simulator platform)
