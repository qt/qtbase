TEMPLATE=subdirs

SUBDIRS = qabstractitemmodel \
    qstringlistmodel \

qtHaveModule(gui): SUBDIRS += \
    qabstractproxymodel \
    qidentityproxymodel \
    qitemselectionmodel \
    qsortfilterproxymodel_recursive \

qtHaveModule(widgets) {
    SUBDIRS += \
        qsortfilterproxymodel

    qtHaveModule(sql): SUBDIRS += \
        qitemmodel
}
