QT += widgets

HEADERS       = digitalclock.h
SOURCES       = digitalclock.cpp \
                main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/widgets/digitalclock
INSTALLS += target
