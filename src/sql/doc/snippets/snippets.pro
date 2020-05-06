TEMPLATE = app
TARGET = sqldatabase_cppsnippet
QT = core sql sql-private

SOURCES += sqldatabase/sqldatabase.cpp \
           code/doc_src_qtsql.cpp \
           code/doc_src_sql-driver.cpp \
           code/src_sql_kernel_qsqldatabase.cpp \
           code/src_sql_kernel_qsqlerror.cpp \
           code/src_sql_kernel_qsqlresult.cpp \
           code/src_sql_kernel_qsqldriver.cpp \
           code/src_sql_models_qsqlquerymodel.cpp

load(qt_common)
