HEADERS += freezetablewidget.h
SOURCES += main.cpp freezetablewidget.cpp
RESOURCES += grades.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/itemviews/frozencolumn
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/itemviews/frozencolumn
INSTALLS += target sources
QT += widgets

