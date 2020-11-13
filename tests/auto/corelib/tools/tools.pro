TEMPLATE=subdirs
SUBDIRS=\
    collections \
    containerapisymmetry \
    qalgorithms \
    qarraydata \
    qbitarray \
    qcache \
    qcommandlineparser \
    qcontiguouscache \
    qcryptographichash \
    qduplicatetracker \
    qeasingcurve \
    qexplicitlyshareddatapointer \
    qflatmap \
    qfreelist \
    qhash \
    qhashfunctions \
    qline \
    qlist \
    qmakearray \
    qmap \
    qmargins \
    qmessageauthenticationcode \
    qoffsetstringarray \
    qpair \
    qpoint \
    qpointf \
    qqueue \
    qrect \
    qringbuffer \
    qscopedpointer \
    qscopedvaluerollback \
    qscopeguard \
    qtaggedpointer \
    qset \
    qsharedpointer \
    qsize \
    qsizef \
    qstl \
    qtimeline \
    qvarlengtharray \
    qversionnumber

darwin: SUBDIRS += qmacautoreleasepool

# QTBUG-88137
android: SUBDIRS -= qtimeline
