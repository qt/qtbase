HEADERS += rsslisting.h
SOURCES += main.cpp rsslisting.cpp
QT += network xml widgets
requires(qtConfig(treewidget))

# install
target.path = $$[QT_INSTALL_EXAMPLES]/xml/rsslisting
INSTALLS += target
