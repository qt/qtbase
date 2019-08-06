TEMPLATE = subdirs
SUBDIRS = \
        qbytearray \
        qchar \
        qlocale \
        qstringbuilder \
        qstringlist

*g++*: SUBDIRS += qstring
