SOURCES += main.cpp \
           myinputpanel.cpp \
           myinputpanelcontext.cpp

HEADERS += myinputpanel.h \
           myinputpanelcontext.h

FORMS   += mainform.ui \
           myinputpanelform.ui

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/inputpanel
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS inputpanel.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/inputpanel
INSTALLS += target sources

symbian: CONFIG += qt_example
