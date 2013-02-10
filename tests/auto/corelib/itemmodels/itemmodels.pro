TEMPLATE=subdirs

SUBDIRS = qabstractitemmodel \
    qabstractproxymodel \
    qidentityproxymodel \
    qitemselectionmodel \
    qstringlistmodel \

qtHaveModule(widgets): SUBDIRS += \
    qitemmodel \
    qsortfilterproxymodel \
