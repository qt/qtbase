requires(qtHaveModule(widgets))

TEMPLATE      = subdirs

SUBDIRS             =   books \
                        drilldown \
                         cachedtable \
                        relationaltablemodel \
                        sqlwidgetmapper

!wince*:  SUBDIRS   +=  masterdetail

!wince*:  SUBDIRS += \
                        querymodel \
                        tablemodel

!cross_compile:{
    contains(QT_BUILD_PARTS, tools):{
        SUBDIRS += sqlbrowser
    }
}

EXAMPLE_FILES = connection.h
