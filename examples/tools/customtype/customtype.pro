HEADERS   = message.h
SOURCES   = main.cpp \
            message.cpp
QT += widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/tools/customcompleter
INSTALLS += target
