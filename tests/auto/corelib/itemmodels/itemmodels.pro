TEMPLATE=subdirs

SUBDIRS = qabstractitemmodel \
    qabstractproxymodel \
    qidentityproxymodel \
    qstringlistmodel \

qtHaveModule(widgets): SUBDIRS += \
    qitemmodel \
    qitemselectionmodel \
    qsortfilterproxymodel \
