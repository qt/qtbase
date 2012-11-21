TEMPLATE    = subdirs
SUBDIRS     = calendar \
              orderform \
              syntaxhighlighter \
              textedit

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/richtext
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS richtext.pro README
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/richtext
INSTALLS += target sources
