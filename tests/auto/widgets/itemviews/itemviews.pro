TEMPLATE=subdirs
SUBDIRS=\
   qabstractitemview \
   qabstractproxymodel \
   qcolumnview \
   qdatawidgetmapper \
   qdirmodel \
   qfileiconprovider \
   qheaderview \
   qidentityproxymodel \
   qitemdelegate \
   qitemeditorfactory \
   qitemselectionmodel \
   qitemview \
   qlistview \
   qlistwidget \
   qsortfilterproxymodel \
   qstandarditem \
   qstandarditemmodel \
   qstringlistmodel \
   qtableview \
   qtablewidget \
   qtreeview \
   qtreewidget \
   qtreewidgetitemiterator \

!contains(QT_CONFIG, private_tests): SUBDIRS -= \
    qcolumnview \
    qlistwidget \

# This test takes too long to run on IRIX, so skip it on that platform
irix-*:SUBDIRS -= qitemview

