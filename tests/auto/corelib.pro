# The tests in this .pro file _MUST_ use QtCore only (i.e. QT=core).
# The test system is allowed to run these tests before the test of Qt has
# been compiled.

TEMPLATE=subdirs
SUBDIRS=\
   collections \
   exceptionsafety \
   q_func_info \
   qanimationgroup \
   qatomicint \
   qatomicpointer \
   qbitarray \
   qbuffer \
   qbytearray \
   qbytearraymatcher \
   qcache \
   qchar \
   qcontiguouscache \
   qcoreapplication \
   qcryptographichash \
   qdate \
   qdatetime \
   qdebug \
   qdiriterator \
   qeasingcurve \
   qelapsedtimer \
   qevent \
   qexplicitlyshareddatapointer \
   qfileinfo \
   qfilesystemwatcher \
   qflags \
   qfreelist \
   qfuture \
   qfuturewatcher \
   qgetputenv \
   qglobal \
   qhash \
   qlibrary \
   qline \
   qmap \
   qmargins \
   qmath \
   qmetatype \
   qmutex \
   qmutexlocker \
   qnumeric \
   qobject \
   qobjectrace \
   qplugin \
   qpluginloader \
   qpoint \
   qprocessenvironment \
   qqueue \
   qrand \
   qreadlocker \
   qreadwritelock \
   qrect \
   qregexp \
   qresourceengine \
   qringbuffer \
   qscopedpointer \
   qscopedvaluerollback \
   qsemaphore \
   qsequentialanimationgroup \
   qset \
   qsharedpointer \
   qsignalspy \
   qsize \
   qsizef \
   qstate \
   qstl \
   qstring \
   qstringbuilder1 \
   qstringbuilder2 \
   qstringbuilder3 \
   qstringbuilder4 \
   qstringlist \
   qstringmatcher \
   qstringref \
   qtconcurrentfilter \
   qtconcurrentiteratekernel \
   qtconcurrentmap \
   qtconcurrentrun \
   qtconcurrentthreadengine \
   qtemporaryfile \
   qtextboundaryfinder \
   qthread \
   qthreadonce \
   qthreadpool \
   qthreadstorage \
   qtime \
   qtimeline \
   qtimer \
   qtmd5 \
   qtokenautomaton \
   qurl \
   quuid \
   qvarlengtharray \
   qvector \
   qwaitcondition \
   qwineventnotifier \
   qwritelocker \
   selftests \
   utf8 \
   qfilesystementry \
   qabstractfileengine

symbian:SUBDIRS -= \
   qtconcurrentfilter \
   qtconcurrentiteratekernel \
   qtconcurrentmap \
   qtconcurrentrun \
   qtconcurrentthreadengine \

!contains(QT_CONFIG, private_tests): SUBDIRS -= \
    qfileinfo \

