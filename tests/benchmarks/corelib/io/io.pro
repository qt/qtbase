TEMPLATE = subdirs
SUBDIRS = \
        qdir \
        qdiriterator \
        qfile \
        qfileinfo \
        qiodevice \
        qtemporaryfile \
        qtextstream

qtConfig(process): SUBDIRS += qprocess
