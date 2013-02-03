TEMPLATE=subdirs
SUBDIRS=\
    qabstractfileengine \
    qbuffer \
    qdatastream \
    qdataurl \
    qdebug \
    qdir \
    qdiriterator \
    qfile \
    largefile \
    qfileinfo \
    qfilesystementry \
    qfilesystemwatcher \
    qiodevice \
    qipaddress \
    qlockfile \
    qnodebug \
    qprocess \
    qprocess-noapplication \
    qprocessenvironment \
    qresourceengine \
    qsettings \
    qsavefile \
    qstandardpaths \
    qtemporarydir \
    qtemporaryfile \
    qtextstream \
    qurl \
    qurlinternal \
    qurlquery \
    qwinoverlappedionotifier \

!win32|wince* {
    SUBDIRS -=\
        qwinoverlappedionotifier
}

!qtHaveModule(network): SUBDIRS -= \
    qfile \
    qiodevice \
    qprocess \
    qtextstream

!contains(QT_CONFIG, private_tests): SUBDIRS -= \
    qabstractfileengine \
    qfileinfo \
    qipaddress \
    qurlinternal

win32:!contains(QT_CONFIG, private_tests): SUBDIRS -= \
    qfilesystementry
