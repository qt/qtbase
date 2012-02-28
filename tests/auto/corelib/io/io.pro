TEMPLATE=subdirs
SUBDIRS=\
    qabstractfileengine \
    qbuffer \
    qdatastream \
    qdebug \
    qdir \
    qdiriterator \
    qfile \
    largefile \
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
    qwinoverlappedionotifier \

!win32|wince* {
    SUBDIRS -=\
        qwinoverlappedionotifier
}

!contains(QT_CONFIG, private_tests): SUBDIRS -= \
    qabstractfileengine \
    qfileinfo

win32:!contains(QT_CONFIG, private_tests): SUBDIRS -= \
    qfilesystementry
