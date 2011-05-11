TEMPLATE=subdirs
SUBDIRS=\
           compiler \
           headersclean \
           maketestselftest \
           #moc \           # FIXME: cannot be built as part of qtbase, since it depends on qtsvg
           uic \
           qmake \
           rcc \
           #atwrapper \     # These tests need significant updating,
           #uiloader \      # they have hardcoded machine names etc.

#contains(QT_CONFIG,qt3support):SUBDIRS+=uic3

