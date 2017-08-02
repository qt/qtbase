# Qt gui library, itemviews

qtConfig(itemviews) {
HEADERS += \
	itemviews/qabstractitemview.h \
    itemviews/qabstractitemview_p.h \
	itemviews/qheaderview.h \
	itemviews/qheaderview_p.h \
	itemviews/qbsptree_p.h \
	itemviews/qabstractitemdelegate.h \
        itemviews/qabstractitemdelegate_p.h \
        itemviews/qitemdelegate.h \
	itemviews/qwidgetitemdata_p.h \
	itemviews/qitemeditorfactory.h \
	itemviews/qitemeditorfactory_p.h \
    itemviews/qstyleditemdelegate.h

SOURCES += \
	itemviews/qabstractitemview.cpp \
	itemviews/qheaderview.cpp \
	itemviews/qbsptree.cpp \
	itemviews/qabstractitemdelegate.cpp \
        itemviews/qitemdelegate.cpp \
	itemviews/qitemeditorfactory.cpp \
    itemviews/qstyleditemdelegate.cpp
}

qtConfig(columnview) {
    HEADERS += \
        itemviews/qcolumnviewgrip_p.h \
        itemviews/qcolumnview.h  \
        itemviews/qcolumnview_p.h

    SOURCES += \
        itemviews/qcolumnview.cpp \
        itemviews/qcolumnviewgrip.cpp
}

qtConfig(datawidgetmapper) {
    HEADERS += itemviews/qdatawidgetmapper.h
    SOURCES += itemviews/qdatawidgetmapper.cpp
}

qtConfig(dirmodel) {
    HEADERS += itemviews/qdirmodel.h
    SOURCES += itemviews/qdirmodel.cpp
}

qtConfig(listview) {
    HEADERS += \
        itemviews/qlistview.h \
        itemviews/qlistview_p.h

    SOURCES += itemviews/qlistview.cpp
}

qtConfig(listwidget) {
    HEADERS += \
        itemviews/qlistwidget.h \
        itemviews/qlistwidget_p.h

    SOURCES += itemviews/qlistwidget.cpp
}

qtConfig(tableview) {
    HEADERS += \
        itemviews/qtableview.h \
        itemviews/qtableview_p.h

    SOURCES += itemviews/qtableview.cpp
}

qtConfig(tablewidget) {
    HEADERS += \
        itemviews/qtablewidget.h \
        itemviews/qtablewidget_p.h

    SOURCES += itemviews/qtablewidget.cpp
}

qtConfig(treeview) {
    HEADERS += \
        itemviews/qtreeview.h \
        itemviews/qtreeview_p.h

    SOURCES += itemviews/qtreeview.cpp
}

qtConfig(treewidget) {
    HEADERS += \
        itemviews/qtreewidget.h \
        itemviews/qtreewidget_p.h \
        itemviews/qtreewidgetitemiterator.h

    SOURCES += \
        itemviews/qtreewidget.cpp \
        itemviews/qtreewidgetitemiterator.cpp
}

HEADERS += \
    itemviews/qfileiconprovider.h \
    itemviews/qfileiconprovider_p.h \

SOURCES +=  \
        itemviews/qfileiconprovider.cpp
