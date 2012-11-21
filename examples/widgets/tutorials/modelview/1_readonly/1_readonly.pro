TARGET =   mv_readonly

TEMPLATE = app

SOURCES += main.cpp \
           mymodel.cpp

HEADERS += mymodel.h


# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/tutorials/modelview/1_readonly
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS 1_readonly.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/tutorials/modelview/1_readonly
INSTALLS += target sources

QT += widgets

simulator: warning(This example might not fully work on Simulator platform)
