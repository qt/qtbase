TEMPLATE = subdirs
SUBDIRS = \
        qbytearray \
        qchar \
        qlocale \
        qstringbuilder \
        qstringlist \
        qregularexpression

*g++*: SUBDIRS += qstring
