SOURCES       = main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/itemviews/dirview
sources.files = $$SOURCES *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/itemviews/dirview
INSTALLS += target sources

QT += widgets
