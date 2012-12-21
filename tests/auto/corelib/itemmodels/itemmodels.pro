TEMPLATE=subdirs

SUBDIRS = qabstractitemmodel \
    qstringlistmodel

qtHaveModule(widgets): SUBDIRS += \
    qabstractproxymodel \
    qidentityproxymodel \
    qitemmodel \
    qitemselectionmodel \
    qsortfilterproxymodel \
