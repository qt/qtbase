TEMPLATE = subdirs
CONFIG += no_docs_target

SUBDIRS = \
    ipc \
    mimetypes \
    serialization \
    tools \
    platform

qtConfig(thread): SUBDIRS += threads
