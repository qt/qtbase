TEMPLATE=subdirs
SUBDIRS=\
   qtmd5 \
   qtokenautomaton \
   baselineexample \

!cross_compile: SUBDIRS += \
   compiler \
   headersclean \
   maketestselftest \
   # atwrapper \ # QTBUG-19452
