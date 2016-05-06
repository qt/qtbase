HEADERS   = message.h \
            window.h
SOURCES   = main.cpp \
            message.cpp \
            window.cpp
QT += widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/corelib/tools/customtypesending
INSTALLS += target
