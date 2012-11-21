HEADERS       = digitalclock.h
SOURCES       = digitalclock.cpp \
                main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/widgets/digitalclock
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS digitalclock.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/widgets/digitalclock
INSTALLS += target sources

QT += widgets
