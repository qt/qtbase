QT += widgets
requires(qtConfig(tableview))

SOURCES = main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/tutorials/widgets/nestedlayouts
INSTALLS += target
