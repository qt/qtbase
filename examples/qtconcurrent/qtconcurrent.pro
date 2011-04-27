TEMPLATE      = subdirs
SUBDIRS       = imagescaling \
                map \
                runfunction \
                wordcount

!wince* {
    SUBDIRS += progressdialog
}

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/qtconcurrent
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS qtconcurrent.pro README
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/qtconcurrent
INSTALLS += target sources

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
