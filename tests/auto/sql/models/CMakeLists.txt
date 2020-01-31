add_subdirectory(qsqlrelationaltablemodel)
add_subdirectory(qsqltablemodel)

if(TARGET Qt::Widgets)
    add_subdirectory(qsqlquerymodel)
endif()
