HEADERS   = block.h \
            renderthread.h \
            window.h
SOURCES   = main.cpp \
            block.cpp \
            renderthread.cpp \
            window.cpp
QT += widgets
requires(qtConfig(filedialog))

# install
target.path = $$[QT_INSTALL_EXAMPLES]/corelib/threads/queuedcustomtype
INSTALLS += target


