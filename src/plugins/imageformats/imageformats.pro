TEMPLATE = subdirs

contains(QT_CONFIG, ico): SUBDIRS += ico
contains(QT_CONFIG, jpeg): SUBDIRS += jpeg
contains(QT_CONFIG, gif): SUBDIRS += gif
