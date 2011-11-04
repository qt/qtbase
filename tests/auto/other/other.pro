TEMPLATE=subdirs
SUBDIRS=\
   # atwrapper \ # QTBUG-19452
   baselineexample \
   compiler \
   headersclean \
   qobjectperformance \
   qtokenautomaton \

cross_compile: SUBDIRS -= \
   atwrapper \
   compiler \
   headersclean \
