HEADERS += freezetablewidget.h
SOURCES += main.cpp freezetablewidget.cpp
RESOURCES += grades.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/itemviews/frozencolumn
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro grades.txt
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/itemviews/frozencolumn
INSTALLS += target sources
QT += widgets

