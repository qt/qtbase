TEMPLATE=subdirs

SUBDIRS = qstringlistmodel

qtHaveModule(gui): SUBDIRS += \
    qabstractitemmodel \
    qabstractproxymodel \
    qidentityproxymodel \
    qitemselectionmodel \
    qsortfilterproxymodel_recursive \

qtHaveModule(widgets) {
    SUBDIRS += \
        qsortfilterproxymodel_regexp \
        qsortfilterproxymodel_regularexpression

    qtHaveModule(sql): SUBDIRS += \
        qitemmodel
}
