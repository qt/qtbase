TEMPLATE=subdirs
SUBDIRS=\
   qfuture \
   qfuturesynchronizer \
   qfuturewatcher \
   qtconcurrentfilter \
   qtconcurrentiteratekernel \
   qtconcurrentmap \
   qtconcurrentresultstore \
   qtconcurrentrun \
   qtconcurrentthreadengine \
   qthreadpool

symbian:SUBDIRS -= \
   qtconcurrentfilter \
   qtconcurrentiteratekernel \
   qtconcurrentmap \
   qtconcurrentrun \
   qtconcurrentthreadengine \
