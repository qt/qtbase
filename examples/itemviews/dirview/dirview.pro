SOURCES       = main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/itemviews/dirview
sources.files = $$SOURCES *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/itemviews/dirview
INSTALLS += target sources

symbian: CONFIG += qt_example
maemo5: CONFIG += qt_example
