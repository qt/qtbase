TEMPLATE=subdirs

SUBDIRS = qabstractitemmodel \
    qabstractproxymodel \
    qidentityproxymodel \
    qitemselectionmodel \
    qsortfilterproxymodel \
    qstringlistmodel

mac: qabstractitemmodel.CONFIG = no_check_target # QTBUG-22748
