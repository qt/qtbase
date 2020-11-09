QT += concurrent widgets network
CONFIG += exceptions
requires(qtConfig(filedialog))

SOURCES += main.cpp imagescaling.cpp \
    downloaddialog.cpp
HEADERS += imagescaling.h \
    downloaddialog.h

target.path = $$[QT_INSTALL_EXAMPLES]/qtconcurrent/imagescaling
INSTALLS += target

FORMS += \
    downloaddialog.ui
