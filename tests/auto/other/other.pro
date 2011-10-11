TEMPLATE=subdirs
SUBDIRS=\
   # atwrapper \ # QTBUG-19452
   baselineexample \
   compiler \
   headersclean \
   qtmd5 \
   qtokenautomaton \

cross_compile: SUBDIRS -= \
   atwrapper \
   compiler \
   headersclean \
