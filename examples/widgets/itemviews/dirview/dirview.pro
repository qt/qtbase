SOURCES       = main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/itemviews/dirview
sources.files = $$SOURCES *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/itemviews/dirview
INSTALLS += target sources

QT += widgets
