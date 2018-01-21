QT += widgets
requires(qtConfig(treeview))

SOURCES       = main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/itemviews/dirview
INSTALLS += target
