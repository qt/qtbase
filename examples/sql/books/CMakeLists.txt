# Generated from books.pro.

#####################################################################
## books Binary:
#####################################################################

add_qt_executable(books
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/sql/books"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/sql/books"
    SOURCES
        bookdelegate.cpp bookdelegate.h
        bookwindow.cpp bookwindow.h bookwindow.ui
        initdb.h
        main.cpp
    INCLUDE_DIRECTORIES
        .
    PUBLIC_LIBRARIES
        Qt::Sql
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)

# Resources:
add_qt_resource(books "books" PREFIX "/" FILES
    images/star.png)


#### Keys ignored in scope 1:.:.:books.pro:<TRUE>:
# TEMPLATE = "app"
