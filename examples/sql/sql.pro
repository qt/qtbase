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

# install
sources.files = connection.h sql.pro README
sources.path = $$[QT_INSTALL_EXAMPLES]/sql
INSTALLS += sources
