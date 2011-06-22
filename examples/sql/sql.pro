TEMPLATE      = subdirs

SUBDIRS             =   books \
                        drilldown
!symbian: SUBDIRS   +=  cachedtable \
                        relationaltablemodel \
                        sqlwidgetmapper

!wince*:  SUBDIRS   +=  masterdetail

!wince*:!symbian: SUBDIRS += \
                        querymodel \
                        tablemodel

!cross_compile:{
    contains(QT_BUILD_PARTS, tools):{
        SUBDIRS += sqlbrowser
    }
}

# install
sources.files = connection.h sql.pro README
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/sql
INSTALLS += sources

symbian: CONFIG += qt_example
maemo5: CONFIG += qt_example
