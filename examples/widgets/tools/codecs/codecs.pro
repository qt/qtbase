QT += widgets
requires(qtConfig(filedialog))

HEADERS      += mainwindow.h \
                previewform.h \
                encodingdialog.h

SOURCES      += main.cpp \
                mainwindow.cpp \
                previewform.cpp \
                encodingdialog.cpp

RESOURCES    += codecs.qrc

EXAMPLE_FILES = encodedfiles

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/tools/codecs
INSTALLS += target
