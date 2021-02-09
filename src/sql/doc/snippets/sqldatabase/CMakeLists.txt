add_library(sqldatabase_snippets OBJECT
    sqldatabase.cpp
)

target_link_libraries(sqldatabase_snippets PRIVATE
    Qt::Core
    Qt::Sql
)
