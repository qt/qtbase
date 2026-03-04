// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtCore/qrangemodeladapter.h>

#ifndef QT_NO_WIDGETS

using namespace Qt::StringLiterals;

#include <QtWidgets/qlistview.h>
#include <QtWidgets/qtableview.h>
#include <QtWidgets/qtreeview.h>
#include <vector>

class Book
{
    Q_GADGET
    Q_PROPERTY(QString title READ title)
    Q_PROPERTY(QString author READ author)
    Q_PROPERTY(QString summary MEMBER m_summary)
    Q_PROPERTY(int rating READ rating WRITE setRating)
public:
    enum Roles
    {
        TitleRole = Qt::UserRole,
        AuthorRole,
        SummaryRole,
        RatingRole,
    };

    Book() = default;
    Book(const QString &title, const QString &author);

    // C++ rule of 0: destructor, as well as copy/move operations
    // provided by the compiler.

    // read-only properties
    QString title() const { return m_title; }
    QString author() const { return m_author; }

    // read/writable property with input validation
    int rating() const { return m_rating; }
    void setRating(int rating)
    {
        m_rating = qBound(0, rating, 5);
    }

private:
    QString m_title;
    QString m_author;
    QString m_summary;
    int m_rating = 0;
};

template <> struct QRangeModel::RowOptions<Book>
{
    static constexpr auto rowCategory = QRangeModel::RowCategory::MultiRoleItem;
};

struct TreeRow;
using Tree = QList<TreeRow *>;

struct TreeRow
{
    QString firstColumn;
    int secondColumn = 0;

    TreeRow() = default;
    explicit TreeRow(const QString &first, int second, std::optional<Tree> children = std::nullopt)
        : firstColumn(first), secondColumn(second), m_children(children)
    {}

    TreeRow *parentRow() const { return m_parent; }
    void setParentRow(TreeRow *parent) { m_parent = parent; }
    const std::optional<Tree> &childRows() const { return m_children; }
    std::optional<Tree> &childRows() { return m_children; }

private:
    TreeRow *m_parent;
    std::optional<Tree> m_children;

    template <size_t I> friend inline decltype(auto) get(const TreeRow &row) {
        static_assert(I < 2);
        return false;
    }
};

namespace std {
    template<> struct tuple_size<TreeRow> : integral_constant<int, 2> {};
    template<> struct tuple_element<0, TreeRow> { using type = QString; };
    template<> struct tuple_element<1, TreeRow> { using type = int; };
}

void construct_and_use()
{
    //! [construct]
    std::vector<int> data = {1, 2, 3, 4, 5};
    QRangeModelAdapter adapter(&data);
    //! [construct]

    //! [class-member-rvalue]
    class Backend
    {
        using Adapter = decltype(QRangeModelAdapter(std::vector<Book>{}));
        Adapter adapter = Adapter(std::vector<Book>{});
    //! [class-member-rvalue]
    };

    class Backend2
    {
    //! [class-member-refwrapper]
        QList<Book> books = { /* ... */ };
        using Adapter = decltype(QRangeModelAdapter(std::ref(books)));
        Adapter adapter = Adapter(std::ref(books));
    };
    //! [class-member-refwrapper]

    //! [use-model]
    QListView listView;
    listView.setModel(adapter.model());
    //! [use-model]
}

void get_and_set()
{
    QListView tableView;
    //! [get-range]
    QList<Book> books = {
        // ...
    };
    QRangeModelAdapter adapter(books);
    tableView.setModel(adapter.model());

    // show UI and where the user can modify the list

    QList<Book> modifiedBooks = adapter.range();
    //! [get-range]

    //! [assign]
    // reset to the original
    adapter = books;
    // or
    adapter.assign(books);
    //! [assign]
}

void dataAccess()
{
    int row = 0;
    int column = 0;
    int path = 0;
    int to = 0;
    int branch = 0;

    QRangeModelAdapter listAdapter(QList<int>{});
    //! [list-data]
    QVariant listItem = listAdapter.data(row);
    //! [list-data]

    QRangeModelAdapter tableAdapter(QList<QList<int>>{});
    //! [table-data]
    QVariant tableItem = tableAdapter.data(row, column);
    //! [table-data]

    QRangeModelAdapter treeAdapter(QList<TreeRow *>{});
    //! [tree-data]
    QVariant treeItem = treeAdapter.data({path, to, branch}, column);
    //! [tree-data]

    //! [multirole-data]
    QRangeModelAdapter listOfBooks(QList<Book>{
        // ~~~
    });
    QString bookTitle = listOfBooks.data(0, Book::TitleRole).toString();
    Book multiRoleItem = listOfBooks.data(0).value<Book>();
    //! [multirole-data]
}

void list_access()
{
    QListView listView;
    {
        //! [list-access]
        QRangeModelAdapter list(std::vector<int>{1, 2, 3, 4, 5});
        listView.setModel(list.model());

        int firstValue = list.at(0); // == 1
        list.at(0) = -1;
        list.at(1) = list.at(4);
        //! [list-access]
    }
    {
        //! [list-access-multirole]
        QRangeModelAdapter books(QList<Book>{
            // ~~~
        });
        Book firstBook = books.at(0);
        Book newBook = {};
        books.at(0) = newBook; // dataChanged() emitted
        //! [list-access-multirole]

        //! [list-access-multirole-member-access]
        QString title = books.at(0)->title();
        //! [list-access-multirole-member-access]

        //! [list-access-multirole-write-back]
        // books.at(0)->setRating(5); - not possible even though 'books' is not const
        firstBook = books.at(0);
        firstBook.setRating(5);
        books.at(0) = firstBook; // dataChanged() emitted
        //! [list-access-multirole-write-back]
    }
}

void table_access()
{
    QTableView tableView;
    {
        //! [table-item-access]
        QRangeModelAdapter table(std::vector<std::vector<double>>{
            {1.0, 2.0, 3.0, 4.0, 5.0},
            {6.0, 7.0, 8.0, 9.0, 10.0},
        });
        tableView.setModel(table.model());

        double value = table.at(0, 2); // value == 3.0
        table.at(0, 2) = value * 2; // table[0, 2] == 6.0
        //! [table-item-access]

        //! [table-row-const-access]
        const auto &constTable = table;
        const std::vector<double> &topRow = constTable.at(0);
        //! [table-row-const-access]

        //! [table-row-access]
        auto lastRow = table.at(table.rowCount() - 1);
        lastRow = { 6.5, 7.5, 8.0, 9.0, 10 }; // emits dataChanged() for entire row
        //! [table-row-access]
    }

    {
        //! [table-mixed-type-access]
        QRangeModelAdapter table(std::vector<std::tuple<int, QString>>{
            // ~~~
        });
        int number = table.at(0, 0)->toInt();
        QString text = table.at(0, 1)->toString();
        //! [table-mixed-type-access]
    }
}

void tree_access()
{
    QTreeView treeView;

    //! [tree-row-access]
    QRangeModelAdapter tree = QRangeModelAdapter(Tree{
        new TreeRow{"Germany", 357002, Tree{
                new TreeRow("Bavaria", 70550)
            },
        },
        new TreeRow{"France", 632702},
    });
    treeView.setModel(tree.model());

    auto germanyData = tree.at(0);
    auto bavariaData = tree.at({0, 0});
    //! [tree-row-access]

    //! [tree-item-access]
    auto germanyName = tree.at(0, 0);
    auto bavariaSize = tree.at({0, 0}, 1);
    //! [tree-item-access]

    //! [tree-row-write]
    // deletes the old row - tree was moved in
    tree.at({0, 0}) = new TreeRow{"Berlin", 892};
    //! [tree-row-write]
}

void read_only()
{
#if 0
    //! [read-only]
    const QStringList strings = {"On", "Off"};
    QRangeModelAdapter adapter(strings);
    adapter.at(0) = "Undecided"; // compile error: return value of 'at' is const
    adapter.insertRow(0); // compiler error: requirements not satisfied
    //! [read-only]
#endif
}

void list_iterate()
{
    QRangeModelAdapter books(QList<Book>{
        // ~~~
    });
    QListView view;
    view.setModel(books.model());

    //! [ranged-for-const-list]
    for (const auto &book : std::as_const(books)) {
        qDebug() << "The book" << book.title()
                 << "written by" << book.author()
                 << "has" << book.rating() << "stars";
    }
    //! [ranged-for-const-list]

    //! [ranged-for-mutable-list]
    for (auto book : books) {
        qDebug() << "The book" << book->title()
                 << "written by" << book->author()
                 << "has" << book->rating() << "stars";

        Book copy = book;
        copy.setRating(copy.rating() + 1);
        book = copy;
    }
    //! [ranged-for-mutable-list]
}

void table_iterate()
{
    //! [ranged-for-const-table]
    QRangeModelAdapter table(std::vector<std::pair<int, QString>>{
        // ~~~
    });

    for (const auto &row : std::as_const(table)) {
        qDebug() << "Number is" << row->first << "and string is" << row->second;
    }
    //! [ranged-for-const-table]

    //! [ranged-for-const-table-items]
    for (const auto &row : std::as_const(table)) {
        for (const auto &item : row) {
            qDebug() << item; // item is a QVariant
        }
    }
    //! [ranged-for-const-table-items]

    //! [ranged-for-mutable-table]
    for (auto row : table) {
        qDebug() << "Number is" << row->first << "and string is" << row->second;
        row = std::make_pair(42, u"forty-two"_s);
    }
    //! [ranged-for-mutable-table]

    //! [ranged-for-mutable-table-items]
    for (auto row : table) {
        for (auto item : row) {
            item = 42;
        }
    }
    //! [ranged-for-mutable-table-items]
}

void tree_iterate()
{
    QRangeModelAdapter tree = QRangeModelAdapter(Tree{
        new TreeRow{"1", 1, Tree{
                new TreeRow("1.1", 11)
            },
        },
        new TreeRow{"2", 2},
    });

    static_assert(std::is_same_v<typename decltype(tree)::DataReference::value_type, QVariant>);

    //! [ranged-for-tree]
    for (auto row : tree) {
        if (row.hasChildren()) {
            for (auto child : row.children()) {
                // ~~~
            }
        }
    }
    //! [ranged-for-tree]
}

#endif
