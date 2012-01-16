TEMPLATE=subdirs

SUBDIRS = qabstractitemmodel \
    qabstractproxymodel \
    qidentityproxymodel \
    qitemmodel \
    qitemselectionmodel \
    qsortfilterproxymodel \
    qstringlistmodel

mac: qabstractitemmodel.CONFIG = no_check_target # QTBUG-22748
