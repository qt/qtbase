TEMPLATE = subdirs
CONFIG += no_docs_target

SUBDIRS = \
    ipc \
    mimetypes \
    serialization \
    tools

qtConfig(thread): SUBDIRS += threads
