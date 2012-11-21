TEMPLATE  = subdirs
SUBDIRS   = part1 part2 part3 part4 part5 part6 part7

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/tutorials/addressbook-fr
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS addressbook-fr.pro README
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/tutorials/addressbook-fr
INSTALLS += target sources
QT += widgets

