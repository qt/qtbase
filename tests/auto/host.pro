TEMPLATE=subdirs
SUBDIRS=\
           compiler \
           headersclean \
           maketestselftest \
           moc \
           uic \
           qmake \
           rcc \
           #atwrapper \     # QTBUG-19452: This test needs to be reworked or discarded
           #uiloader \      # QTBUG-19453: this test has hardcoded machine names etc.

