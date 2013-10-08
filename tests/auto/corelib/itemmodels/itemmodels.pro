TEMPLATE=subdirs

SUBDIRS = qabstractitemmodel \
    qstringlistmodel \

qtHaveModule(gui): SUBDIRS += \
    qabstractproxymodel \
    qidentityproxymodel \
    qitemselectionmodel \

qtHaveModule(widgets): SUBDIRS += \
    qitemmodel \
    qsortfilterproxymodel \
