HEADERS       = tetrixboard.h \
                tetrixpiece.h \
                tetrixwindow.h
SOURCES       = main.cpp \
                tetrixboard.cpp \
                tetrixpiece.cpp \
                tetrixwindow.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/widgets/tetrix
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS tetrix.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/widgets/tetrix
INSTALLS += target sources

QT += widgets

