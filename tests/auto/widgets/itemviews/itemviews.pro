TEMPLATE=subdirs
SUBDIRS=\
   qabstractitemview \
   qcolumnview \
   qdatawidgetmapper \
   qfileiconprovider \
   qheaderview \
   qitemdelegate \
   qitemeditorfactory \
   qitemview \
   qlistview \
   qlistwidget \
   qtableview \
   qtablewidget \
   qtreeview \
   qtreewidget \
   qtreewidgetitemiterator \

!qtConfig(private_tests): SUBDIRS -= \
    qcolumnview \
    qlistwidget \
