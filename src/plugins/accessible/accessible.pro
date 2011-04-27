TEMPLATE = subdirs

contains(QT_CONFIG, accessibility) {
     SUBDIRS += widgets 
     contains(QT_CONFIG, qt3support):SUBDIRS += compat
}
