QT += core gui widgets
requires(qtConfig(treeview))
TARGET = storageview
TEMPLATE = app
SOURCES += storagemodel.cpp \
    main.cpp
HEADERS += \
    storagemodel.h

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/itemviews/storageview
INSTALLS += target
