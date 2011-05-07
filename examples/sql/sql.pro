TEMPLATE      = subdirs

SUBDIRS             =   drilldown
!symbian: SUBDIRS   +=  cachedtable \
                        relationaltablemodel \
                        sqlwidgetmapper

!wince*:  SUBDIRS   +=  masterdetail

!wince*:!symbian: SUBDIRS += \
                        querymodel \
                        tablemodel


# install
sources.files = connection.h sql.pro README
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/sql
INSTALLS += sources

symbian: CONFIG += qt_example
QT += widgets
