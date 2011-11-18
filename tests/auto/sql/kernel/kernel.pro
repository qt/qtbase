TEMPLATE=subdirs
SUBDIRS=\
   qsqlfield \
   qsqldatabase \
   qsqlerror \
   qsqldriver \
   qsqlquery \
   qsqlrecord \
   qsqlthread \
   qsql \

mac: qsql.CONFIG = no_check_target # QTBUG-22811
