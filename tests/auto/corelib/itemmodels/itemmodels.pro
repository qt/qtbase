TEMPLATE=subdirs

SUBDIRS = qabstractitemmodel

mac: qabstractitemmodel.CONFIG = no_check_target # QTBUG-22748
