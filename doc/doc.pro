TEMPLATE = aux

global_docs.files = \
    $$PWD/global \
    $$PWD/config
global_docs.path = $$[QT_INSTALL_DOCS]
INSTALLS += global_docs
!prefix_build:!equals(OUT_PWD, $$PWD): \
    COPIES += global_docs
