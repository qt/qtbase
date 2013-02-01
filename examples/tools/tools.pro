requires(qtHaveModule(widgets))

TEMPLATE      = subdirs
CONFIG       += ordered
SUBDIRS       = contiguouscache \
                customtype \
                customtypesending
