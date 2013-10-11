QT += widgets

HEADERS       = iconpreviewarea.h \
                iconsizespinbox.h \
                imagedelegate.h \
                mainwindow.h
SOURCES       = iconpreviewarea.cpp \
                iconsizespinbox.cpp \
                imagedelegate.cpp \
                main.cpp \
                mainwindow.cpp

EXAMPLE_FILES = images/*

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/widgets/icons
INSTALLS += target


wince*: {
    imageFiles.files = images/*
    wincewm*: {
        imageFiles.path = "/My Documents/My Pictures"
    } else {
        imageFiles.path    = images
    }
    DEPLOYMENT += imageFiles
}
