TEMPLATE=subdirs

SUBDIRS = qstringlistmodel

qtHaveModule(gui): SUBDIRS += \
    qabstractitemmodel \
    qabstractproxymodel \
    qconcatenatetablesproxymodel \
    qidentityproxymodel \
    qitemselectionmodel \
    qsortfilterproxymodel_recursive \
    qtransposeproxymodel \

qtHaveModule(widgets) {
    SUBDIRS += \
        qsortfilterproxymodel_regexp \
        qsortfilterproxymodel_regularexpression

    qtHaveModule(sql): SUBDIRS += \
        qitemmodel
}
