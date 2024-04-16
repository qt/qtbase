TEMPLATE = subdirs
CONFIG += no_docs_target

SUBDIRS = \
    ipc \
    mimetypes \
    serialization \
    tools \
    platform \
    time

qtConfig(thread): SUBDIRS += threads
