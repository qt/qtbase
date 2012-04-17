TEMPLATE = subdirs
SUBDIRS = \
        containers-associative \
        containers-sequential \
        qbytearray \
        qcontiguouscache \
        qlist \
        qmap \
        qrect \
        qregexp \
        qstring \
        qstringbuilder \
        qstringlist \
        qvector \
        qalgorithms

!*g++*: SUBDIRS -= qstring
