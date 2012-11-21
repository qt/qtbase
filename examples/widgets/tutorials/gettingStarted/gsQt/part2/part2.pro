
QT += widgets
SOURCES = main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/tutorials/gettingStarted/gsQt/part2
sources.files = $$SOURCES *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/tutorials/gettingStarted/gsQt/part2
INSTALLS += target sources

