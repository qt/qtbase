TEMPLATE=subdirs
SUBDIRS=\
   # atwrapper \ # QTBUG-19452
   baselineexample \
   compiler \
   headersclean \
   qtokenautomaton \

cross_compile: SUBDIRS -= \
   atwrapper \
   compiler \
   headersclean \
