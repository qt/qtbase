TEMPLATE = subdirs
SUBDIRS = \
        qbytearray \
        qchar \
        qlocale \
        qregexp \
        qstringbuilder \
        qstringlist

*g++*: SUBDIRS += qstring
