TEMPLATE=subdirs

SUBDIRS = qabstractitemmodel \
    qstringlistmodel \

qtHaveModule(gui): SUBDIRS += \
    qabstractproxymodel \
    qidentityproxymodel \
    qitemselectionmodel \

qtHaveModule(widgets) {
    SUBDIRS += \
        qsortfilterproxymodel

    qtHaveModule(sql): SUBDIRS += \
        qitemmodel
}
