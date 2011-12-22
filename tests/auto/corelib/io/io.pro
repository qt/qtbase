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
    qnodebug \
    qprocess \
    qprocessenvironment \
    qresourceengine \
    qsettings \
    qstandardpaths \
    qtemporarydir \
    qtemporaryfile \
    qtextstream \
    qurl \

!contains(QT_CONFIG, private_tests): SUBDIRS -= \
    qfileinfo

win32:!contains(QT_CONFIG, private_tests): SUBDIRS -= \
    qfilesystementry
