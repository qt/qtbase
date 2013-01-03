QT += widgets

HEADERS         = highlighter.h \
                  mainwindow.h
SOURCES         = highlighter.cpp \
                  mainwindow.cpp \
                  main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/richtext/syntaxhighlighter
INSTALLS += target


wince*: {
   addFiles.files = main.cpp mainwindow.cpp
   addFiles.path = .
   DEPLOYMENT += addFiles
}
