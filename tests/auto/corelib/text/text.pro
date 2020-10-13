TEMPLATE = subdirs

SUBDIRS = \
    qbytearray \
    qbytearrayapisymmetry \
    qbytearraylist \
    qbytearraymatcher \
    qbytearrayview \
    qbytedatabuffer \
    qchar \
    qcollator \
    qlatin1string \
    qlocale \
    qregularexpression \
    qstring \
    qstring_no_cast_from_bytearray \
    qstringapisymmetry \
    qstringbuilder \
    qstringconverter \
    qstringiterator \
    qstringlist \
    qstringmatcher \
    qstringtokenizer \
    qstringview \
    qtextboundaryfinder

# QTBUG-87414
android: SUBDIRS -= \
    qlocale
