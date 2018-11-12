TEMPLATE=subdirs
SUBDIRS=\
    qabstractfileengine \
    qbuffer \
    qdataurl \
    qdebug \
    qdir \
    qdiriterator \
    qfile \
    largefile \
    qfileinfo \
    qfileselector \
    qfilesystemmetadata \
    qfilesystementry \
    qfilesystemwatcher \
    qiodevice \
    qipaddress \
    qlockfile \
    qloggingcategory \
    qloggingregistry \
    qnodebug \
    qprocess \
    qprocess-noapplication \
    qprocessenvironment \
    qresourceengine \
    qsettings \
    qsavefile \
    qstandardpaths \
    qstorageinfo \
    qtemporarydir \
    qtemporaryfile \
    qurl \
    qurlinternal \
    qurlquery \

!qtHaveModule(gui): SUBDIRS -= \
    qsettings

!qtHaveModule(network): SUBDIRS -= \
    qiodevice \
    qprocess

!qtHaveModule(concurrent): SUBDIRS -= \
    qdebug \
    qlockfile \
    qurl

!qtConfig(private_tests): SUBDIRS -= \
    qabstractfileengine \
    qfileinfo \
    qipaddress \
    qurlinternal \
    qloggingregistry

win32:!qtConfig(private_tests): SUBDIRS -= \
    qfilesystementry

!qtConfig(filesystemwatcher): SUBDIRS -= \
    qfilesystemwatcher

!qtConfig(processenvironment): SUBDIRS -= \
    qprocessenvironment

!qtConfig(process): SUBDIRS -= \
    qprocess \
    qprocess-noapplication

!qtConfig(settings): SUBDIRS -= \
    qsettings

winrt: SUBDIRS -= \
    qstorageinfo

android: SUBDIRS -= \
    qprocess \
    qdir \
    qresourceengine
