TEMPLATE    = subdirs
SUBDIRS     = calendar \
              orderform \
              syntaxhighlighter \
              textedit

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/richtext
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS richtext.pro README
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/richtext
INSTALLS += target sources
