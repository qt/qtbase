TEMPLATE = subdirs
SUBDIRS = \
        containers-associative \
        containers-sequential \
        qbytearray \
        qcontiguouscache \
        qcryptographichash \
        qdatetime \
        qlist \
        qlocale \
        qmap \
        qrect \
        qringbuffer \
        qstack \
        qstring \
        qstringbuilder \
        qstringlist \
        qvector \
        qalgorithms

!*g++*: SUBDIRS -= qstring
