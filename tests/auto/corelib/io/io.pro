TEMPLATE=subdirs
SUBDIRS=\
    qabstractfileengine \
    qbuffer \
    qdatastream \
    qdebug \
    qdir \
    qdiriterator \
    qfile \
    qfileinfo \
    qfilesystementry \
    qfilesystemwatcher \
    qiodevice \
    qprocess \
    qprocessenvironment \
    qresourceengine \
    qsettings \
    qstandardpaths \
    qtemporaryfile \
    qtextstream \
    qurl \

!contains(QT_CONFIG, private_tests): SUBDIRS -= \
    qfileinfo
