QT += widgets
requires(qtConfig(combobox))

HEADERS     = window.h
SOURCES     = main.cpp \
              window.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/widgets/lineedits
INSTALLS += target
