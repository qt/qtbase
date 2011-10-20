# The tests in this .pro file _MUST_ use QtCore and QtNetwork only
# (i.e. QT=core network).
# The test system is allowed to run these tests before the rest of Qt has
# been compiled.
TEMPLATE=subdirs
SUBDIRS=\
    network \
    qobjectperformance 
