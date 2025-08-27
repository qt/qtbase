// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtCore/qrangemodel.h>

#ifndef QT_NO_WIDGETS

#include <QtWidgets/qapplication.h>
#include <QtWidgets/qlistview.h>
#include <QtWidgets/qtableview.h>
#include <QtWidgets/qtreeview.h>

#include <array>
#include <vector>

void array()
{
    QListView listView;

    //! [array]
    std::array<int, 5> numbers = {1, 2, 3, 4, 5};
    QRangeModel model(numbers);
    listView.setModel(&model);
    //! [array]
}

void construct_by()
{
    std::vector<int> numbers = {1, 2, 3, 4, 5};

    {
        //! [value]
        QRangeModel model(numbers);
        //! [value]
    }

    {
        //! [pointer]
        QRangeModel model(&numbers);
        //! [pointer]
    }

    {
        //! [reference_wrapper]
        QRangeModel model(std::ref(numbers));
        //! [reference_wrapper]
    }

    {
        //! [smart_pointer]
        auto shared_numbers = std::make_shared<std::vector<int>>(numbers);
        QRangeModel model(shared_numbers);
        //! [smart_pointer]
    }
}

void const_array()
{
    //! [const_array]
    const std::array<int, 5> numbers = {1, 2, 3, 4, 5};
    //! [const_array]
    QRangeModel model(numbers);
}

void const_values()
{
    //! [const_values]
    std::array<const int, 5> numbers = {1, 2, 3, 4, 5};
    //! [const_values]
    QRangeModel model(numbers);
}

void const_ref()
{
    std::vector<int> numbers = {1, 2, 3, 4, 5};
    //! [const_ref]
    QRangeModel model(std::cref(numbers));
    //! [const_ref]
}

void list_of_int()
{
    //! [list_of_int]
    QList<int> numbers = {1, 2, 3, 4, 5};
    QRangeModel model(numbers); // columnCount() == 1
    QListView listView;
    listView.setModel(&model);
    //! [list_of_int]
}

void grid_of_numbers()
{
    //! [grid_of_numbers]
    std::vector<std::vector<int>> gridOfNumbers = {
        {1, 2, 3, 4, 5},
        {6, 7, 8, 9, 10},
        {11, 12, 13, 14, 15},
    };
    QRangeModel model(&gridOfNumbers); // columnCount() == 5
    QTableView tableView;
    tableView.setModel(&model);
    //! [grid_of_numbers]
}

void pair_int_QString()
{
    //! [pair_int_QString]
    using TableRow = std::tuple<int, QString>;
    QList<TableRow> numberNames = {
        {1, "one"},
        {2, "two"},
        {3, "three"}
    };
    QRangeModel model(&numberNames); // columnCount() == 2
    QTableView tableView;
    tableView.setModel(&model);
    //! [pair_int_QString]
}

#if defined(__cpp_concepts) && defined(__cpp_lib_forward_like)
//! [tuple_protocol]
struct Book
{
    QString title;
    QString author;
    QString summary;
    int rating = 0;

    template <size_t I, typename T>
        requires ((I <= 3) && std::is_same_v<std::remove_cvref_t<T>, Book>)
    friend inline decltype(auto) get(T &&book)
    {
        if constexpr (I == 0)
            return std::as_const(book.title);
        else if constexpr (I == 1)
            return std::as_const(book.author);
        else if constexpr (I == 2)
            return std::forward_like<T>(book.summary);
        else if constexpr (I == 3)
            return std::forward_like<T>(book.rating);
    }
};

namespace std {
    template <> struct tuple_size<Book> : std::integral_constant<size_t, 4> {};
    template <size_t I> struct tuple_element<I, Book>
    { using type = decltype(get<I>(std::declval<Book>())); };
}
//! [tuple_protocol]
#endif // __cpp_concepts && forward_like

namespace gadget {
//! [gadget]
class Book
{
    Q_GADGET
    Q_PROPERTY(QString title READ title)
    Q_PROPERTY(QString author READ author)
    Q_PROPERTY(QString summary MEMBER m_summary)
    Q_PROPERTY(int rating READ rating WRITE setRating)
public:
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
//! [gadget]
} // namespace gadget

namespace Object
{
//! [object_0]
class Entry : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString display READ display WRITE setDisplay NOTIFY displayChanged)
    Q_PROPERTY(QIcon decoration READ decoration WRITE setDecoration NOTIFY decorationChanged)
    Q_PROPERTY(QString toolTip READ toolTip WRITE setToolTip NOTIFY toolTipChanged)

public:
    Entry() = default;

    QString display() const
    {
        return m_display;
    }

    void setDisplay(const QString &display)
    {
        if (m_display == display)
            return;
        m_display = display;
        emit displayChanged(m_display);
    }

signals:
    void displayChanged(const QString &);
//! [object_0]

private:
    QString m_display;

//! [object_1]
};
//! [object_1]

void vector_of_objects()
{
    //! [vector_of_objects_0]
    std::vector<std::shared_ptr<Entry>> entries = {
    //! [vector_of_objects_0]
        std::make_shared<Entry>(),
    //! [vector_of_objects_1]
    };
    //! [vector_of_objects_1]

    //! [vector_of_objects_2]
    QRangeModel model(std::ref(entries));
    QListView listView;
    listView.setModel(&model);
    //! [vector_of_objects_2]
}

} // namespace object

using Entry = Object::Entry;
//! [vector_of_multirole_objects_0]
template <>
struct QRangeModel::RowOptions<Entry>
{
    static constexpr auto rowCategory = QRangeModel::RowCategory::MultiRoleItem;
};
//! [vector_of_multirole_objects_0]

namespace Object
{

void vector_of_multirole_objects()
{
    //! [vector_of_multirole_objects_1]
    std::vector<std::shared_ptr<Entry>> entries = {
        std::make_shared<Entry>(),
    //! [vector_of_multirole_objects_1]
    //! [vector_of_multirole_objects_2]
    };

    QRangeModel model(std::ref(entries));
    //! [vector_of_multirole_objects_2]
}

} // namespace object

namespace Subclass
{

//! [subclass_header]
class NumbersModel : public QRangeModel
{
    std::vector<int> m_numbers;

public:
    NumbersModel(const std::vector<int> &numbers)
        : QRangeModel(std::ref(m_numbers))
        , m_numbers(numbers)
    {
    }
//! [subclass_header]
//! [subclass_API]
    void setNumber(int idx, int number)
    {
        setData(index(idx, 0), QVariant::fromValue(number));
    }

    int number(int idx) const
    {
        return m_numbers.at(idx);
    }
};
//! [subclass_API]

} // namespace Subclass

namespace tree_protocol
{
//! [tree_protocol_0]
class TreeRow;

using Tree = std::vector<TreeRow>;
//! [tree_protocol_0]

//! [tree_protocol_1]
class TreeRow
{
    Q_GADGET
    // properties

    TreeRow *m_parent;
    std::optional<Tree> m_children;

public:
    TreeRow() = default;

    // rule of 0: copy, move, and destructor implicitly defaulted
//! [tree_protocol_1]

    friend struct TreeTraversal;
    TreeRow(const QString &) {}

//! [tree_protocol_2]
    // tree traversal protocol implementation
    const TreeRow *parentRow() const { return m_parent; }
    const std::optional<Tree> &childRows() const { return m_children; }
//! [tree_protocol_2]
//! [tree_protocol_3]
    void setParentRow(TreeRow *parent) { m_parent = parent; }
    std::optional<Tree> &childRows() { return m_children; }
//! [tree_protocol_3]
//! [tree_protocol_4]
    // Helper to assembly a tree of rows, not used by QRangeModel
    template <typename ...Args>
    TreeRow &addChild(Args &&...args)
    {
        if (!m_children)
            m_children.emplace(Tree{});
        auto &child = m_children->emplace_back(std::forward<Args>(args)...);
        child.m_parent = this;
        return child;
    }
};
//! [tree_protocol_4]

void tree_protocol()
{
    //! [tree_protocol_5]
    Tree tree = {
        {"..."},
        {"..."},
        {"..."},
    };

    // each toplevel row has three children
    tree[0].addChild("...");
    tree[0].addChild("...");
    tree[0].addChild("...");

    tree[1].addChild("...");
    tree[1].addChild("...");
    tree[1].addChild("...");

    tree[2].addChild("...");
    tree[2].addChild("...");
    tree[2].addChild("...");
    //! [tree_protocol_5]

    //! [tree_protocol_6]
    // instantiate the model with a pointer to the tree, not a copy!
    QRangeModel model(&tree);
    QTreeView view;
    view.setModel(&model);
    //! [tree_protocol_6]
}

//! [explicit_tree_protocol_0]
struct TreeTraversal
{
    TreeRow newRow() const { return TreeRow{}; }
    const TreeRow *parentRow(const TreeRow &row) const { return row.m_parent; }
    void setParentRow(TreeRow &row, TreeRow *parent) const { row.m_parent = parent; }
    const std::optional<Tree> &childRows(const TreeRow &row) const { return row.m_children; }
    std::optional<Tree> &childRows(TreeRow &row) const { return row.m_children; }
};
//! [explicit_tree_protocol_0]
void explicit_tree_protocol()
{
    Tree tree;
    //! [explicit_tree_protocol_1]
    QRangeModel model(&tree, TreeTraversal{});
    //! [explicit_tree_protocol_1]
}
} // namespace tree_protocol

namespace tree_of_pointers
{
//! [tree_of_pointers_0]
struct TreeRow;
using Tree = std::vector<TreeRow *>;
//! [tree_of_pointers_0]

//! [tree_of_pointers_1]
struct TreeRow
{
    Q_GADGET
public:
    TreeRow(const QString &value = {})
        : m_value(value)
    {}
    ~TreeRow()
    {
        if (m_children)
            qDeleteAll(*m_children);
    }

    // move-only
    TreeRow(TreeRow &&) = default;
    TreeRow &operator=(TreeRow &&) = default;

    // helper to populate
    template <typename ...Args>
    TreeRow *addChild(Args &&...args)
    {
        if (!m_children)
            m_children.emplace(Tree{});
        auto *child = m_children->emplace_back(new TreeRow(std::forward<Args>(args)...));
        child->m_parent = this;
        return child;
    }

private:
    friend struct TreeTraversal;
    QString m_value;
    std::optional<Tree> m_children;
    TreeRow *m_parent = nullptr;
};
//! [tree_of_pointers_1]

Tree make_tree_of_pointers()
{
    //! [tree_of_pointers_2]
    Tree tree = {
        new TreeRow("1"),
        new TreeRow("2"),
        new TreeRow("3"),
        new TreeRow("4"),
    };
    tree[0]->addChild("1.1");
    tree[1]->addChild("2.1");
    tree[2]->addChild("3.1")->addChild("3.1.1");
    tree[3]->addChild("4.1");
    //! [tree_of_pointers_2]
    return tree;
}

//! [tree_of_pointers_3]
struct TreeTraversal
{
    TreeRow *newRow() const { return new TreeRow; }
    void deleteRow(TreeRow *row) { delete row; }

    const TreeRow *parentRow(const TreeRow &row) const { return row.m_parent; }
    void setParentRow(TreeRow &row, TreeRow *parent) { row.m_parent = parent; }
    const std::optional<Tree> &childRows(const TreeRow &row) const { return row.m_children; }
    std::optional<Tree> &childRows(TreeRow &row) { return row.m_children; }
};
//! [tree_of_pointers_3]

//! [tree_of_pointers_4]
int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    Tree tree = make_tree_of_pointers();

    QRangeModel model(std::move(tree), TreeTraversal{});
    QTreeView treeView;
    treeView.setModel(&model);
    treeView.show();

    return app.exec();
}
//! [tree_of_pointers_4]

} // namespace tree_of_pointers

void color_map()
{
    //! [color_map]
    using ColorEntry = QMap<Qt::ItemDataRole, QVariant>;

    const QStringList colorNames = QColor::colorNames();
    QList<ColorEntry> colors;
    colors.reserve(colorNames.size());
    for (const QString &name : colorNames) {
        const QColor color = QColor::fromString(name);
        colors << ColorEntry{{Qt::DisplayRole, name},
                            {Qt::DecorationRole, color},
                            {Qt::ToolTipRole, color.name()}};
    }
    QRangeModel colorModel(colors);
    QListView list;
    list.setModel(&colorModel);
    //! [color_map]
}

namespace multirole_gadget {
//! [color_gadget_decl]
class ColorEntry
{
    Q_GADGET
    Q_PROPERTY(QString display MEMBER m_colorName)
    Q_PROPERTY(QColor decoration READ decoration)
    Q_PROPERTY(QString toolTip READ toolTip)
public:
//! [color_gadget_decl]
//! [color_gadget_impl]
    ColorEntry(const QString &color = {})
        : m_colorName(color)
    {}

    QColor decoration() const
    {
        return QColor::fromString(m_colorName);
    }
    QString toolTip() const
    {
        return QColor::fromString(m_colorName).name();
    }

private:
    QString m_colorName;
//! [color_gadget_impl]
//! [color_gadget_end]
};
//! [color_gadget_end]
}

using ColorEntry = multirole_gadget::ColorEntry;
//! [color_gadget_multi_role_gadget]

template <>
struct QRangeModel::RowOptions<ColorEntry>
{
    static constexpr auto rowCategory = QRangeModel::RowCategory::MultiRoleItem;
};
//! [color_gadget_multi_role_gadget]

namespace multirole_gadget {
void color_table() {
    //! [color_gadget_table]
    QList<QList<ColorEntry>> colorTable;

    // ...

    QRangeModel colorModel(colorTable);
    QTableView table;
    table.setModel(&colorModel);
    //! [color_gadget_table]
}

void color_list_multi_role() {
    //! [color_gadget_multi_role]
    const QStringList colorNames = QColor::colorNames();
    QList<ColorEntry> colors;
    colors.reserve(colorNames.size());
    for (const QString &name : colorNames)
        colors << name;

    QRangeModel colorModel(colors);
    QListView list;
    list.setModel(&colorModel);
    //! [color_gadget_multi_role]
}

void color_list_single_column() {
    //! [color_gadget_single_column]
    const QStringList colorNames = QColor::colorNames();
    QList<std::tuple<ColorEntry>> colors;

    // ...

    QRangeModel colorModel(colors);
    QListView list;
    list.setModel(&colorModel);
    //! [color_gadget_single_column]

    {
        //! [color_gadget_single_column_access_get]
        ColorEntry firstEntry = std::get<0>(colors.at(0));
        //! [color_gadget_single_column_access_get]
    }
    {
        //! [color_gadget_single_column_access_sb]
        auto [firstEntry] = colors.at(0);
        //! [color_gadget_single_column_access_sb]
    }
}
} // namespace multirole_gadget

#endif // QT_NO_WIDGETS
