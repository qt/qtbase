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
        qsortfilterproxymodel_regexp \
        qsortfilterproxymodel_regularexpression

    qtHaveModule(sql): SUBDIRS += \
        qitemmodel
}
