TEMPLATE=subdirs

SUBDIRS = qabstractitemmodel \
    qstringlistmodel

!contains(QT_CONFIG, no-widgets): SUBDIRS += \
    qabstractproxymodel \
    qidentityproxymodel \
    qitemmodel \
    qitemselectionmodel \
    qsortfilterproxymodel \
