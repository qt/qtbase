TEMPLATE=subdirs
SUBDIRS=\
   qsqlquerymodel \
   qsqlrelationaltablemodel \
   qsqltablemodel \

contains(QT_CONFIG, no-widgets): SUBDIRS -= \
   qsqlquerymodel
