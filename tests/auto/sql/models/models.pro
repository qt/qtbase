TEMPLATE=subdirs
SUBDIRS=\
   qsqlquerymodel \
   qsqlrelationaltablemodel \
   qsqltablemodel \

!qtHaveModule(widgets): SUBDIRS -= \
   qsqlquerymodel
