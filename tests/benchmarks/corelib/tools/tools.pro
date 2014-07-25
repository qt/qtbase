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
        qregexp \
        qringbuffer \
        qstring \
        qstringbuilder \
        qstringlist \
        qvector \
        qalgorithms

!*g++*: SUBDIRS -= qstring
