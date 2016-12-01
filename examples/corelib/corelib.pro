TEMPLATE = subdirs
CONFIG += no_docs_target

SUBDIRS = \
    ipc \
    mimetypes \
    tools

!emscripten: SUBDIRS += \
   threads \
   json


