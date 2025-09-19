// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qobject.h>
#include <QtCore/qrangemodel.h>
#include <QtCore/qstring.h>
#include <QtGui/qcolor.h>

#include <list>
#include <memory>
#include <optional>
#include <tuple>
#include <vector>

using namespace Qt::StringLiterals;

#if defined(__cpp_lib_ranges)
#include <ranges>
#endif

template <typename T>
auto asUPtr(T&& model) {
    return std::make_unique<std::remove_reference_t<T>>(std::forward<T>(model));
}

template <typename T>
auto asSPtr(T&& model) {
    return std::make_shared<std::remove_reference_t<T>>(std::forward<T>(model));
}

class Item
{
    Q_GADGET
    Q_PROPERTY(QString display READ display WRITE setDisplay)
    Q_PROPERTY(QColor decoration READ decoration WRITE setDecoration)
    Q_PROPERTY(QString toolTip READ toolTip WRITE setToolTip)
public:
    Item() = default;

    Item(const QString &display, QColor decoration, const QString &toolTip)
        : m_display(display), m_decoration(decoration), m_toolTip(toolTip)
    {
    }

    QString display() const { return m_display; }
    void setDisplay(const QString &display) { m_display = display; }
    QColor decoration() const { return m_decoration; }
    void setDecoration(QColor decoration) { m_decoration = decoration; }
    QString toolTip() const { return m_toolTip.isEmpty() ? display() : m_toolTip; }
    void setToolTip(const QString &toolTip) { m_toolTip = toolTip; }

private:
    QString m_display;
    QColor m_decoration;
    QString m_toolTip;
};

class MultiRoleGadget
{
    Q_GADGET
    Q_PROPERTY(QString display MEMBER m_display)
    Q_PROPERTY(int number READ number WRITE setNumber)
    Q_PROPERTY(QColor decoration MEMBER m_decoration)
    Q_PROPERTY(QVariant user MEMBER m_user)
public:
    int number() const { return m_number; }
    void setNumber(int number) { m_number = number; }

    QString m_display;
    QColor m_decoration;
    QVariant m_user;
    int m_number = 0;
};

template <>
struct QRangeModel::RowOptions<MultiRoleGadget>
{
    static constexpr auto rowCategory = QRangeModel::RowCategory::MultiRoleItem;
};

class Object : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString string READ string WRITE setString)
    Q_PROPERTY(int number READ number WRITE setNumber)
public:
    using QObject::QObject;

    QString string() const { return m_string; }
    void setString(const QString &string) { m_string = string; }
    int number() const { return m_number; }
    void setNumber(int number) { m_number = number; }

private:
    // note: default values need to be convertible to each other
    QString m_string = "1234";
    int m_number = 42;
};

// a class that can be both and requires disambiguation
class MetaObjectTuple : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString display MEMBER m_string)
    Q_PROPERTY(int number MEMBER m_number)
public:
    using QObject::QObject;

private:
    QString m_string = "4321";
    int m_number = 24;

    template <size_t I, typename G,
        std::enable_if_t<(I < 2), bool> = true,
        std::enable_if_t<std::is_same_v<q20::remove_cvref_t<G>, MetaObjectTuple>, bool> = true
    >
    friend inline decltype(auto) get(G &&item)
    {
        if constexpr (I == 0)
            return q23::forward_like<G>(item.m_string);
        else if constexpr (I == 1)
            return q23::forward_like<G>(item.m_number);
    }
};

namespace std {
    template <> struct tuple_size<MetaObjectTuple> : std::integral_constant<size_t, 2> {};
    template <> struct tuple_element<0, MetaObjectTuple> { using type = QString; };
    template <> struct tuple_element<1, MetaObjectTuple> { using type = int; };
}

struct Row
{
    Item m_item;
    int m_number;
    QString m_description;

    template <size_t I, typename RowType,
        std::enable_if_t<(I < 3), bool> = true,
        std::enable_if_t<std::is_same_v<q20::remove_cvref_t<RowType>, Row>, bool> = true
    >
    friend inline decltype(auto) get(RowType &&item)
    {
        if constexpr (I == 0)
            return q23::forward_like<RowType>(item.m_item);
        else if constexpr (I == 1)
            return q23::forward_like<RowType>(item.m_number);
        else // if constexpr (I == 2)
            return q23::forward_like<RowType>(item.m_description);
    }
};

namespace std {
    template <> struct tuple_size<Row> : std::integral_constant<size_t, 3> {};
    template <> struct tuple_element<0, Row> { using type = Item; };
    template <> struct tuple_element<1, Row> { using type = int; };
    template <> struct tuple_element<2, Row> { using type = QString; };
}

struct ConstRow
{
    QString value;

    template<size_t I,
        std::enable_if_t<I == 0, bool> = true
    >
    friend inline decltype(auto) get(const ConstRow &row)
    {
        if constexpr (I == 0)
            return row.value;
    }
};

namespace std {
    template <> struct tuple_size<ConstRow> : std::integral_constant<size_t, 1> {};
    template <> struct tuple_element<0, ConstRow> { using type = QString; };
}

struct tree_row;
using value_tree = std::vector<tree_row>;
using pointer_tree = QList<tree_row *>;

struct tree_row
{
public:
    tree_row(const QString &value = {}, const QString &description = {})
        : m_value(value), m_description(description)
    {}

    ~tree_row()
    {
        if (m_childrenPointers)
            qDeleteAll(*m_childrenPointers);
    }

    tree_row(tree_row &&other) = default;
    tree_row &operator=(tree_row &&other) = default;

    QString &value() { return m_value; }
    const QString &value() const { return m_value; }
    QString &description() { return m_description; }
    const QString &description() const { return m_description; }

    template <typename ...Args>
    tree_row &addChild(Args&& ...args)
    {
        if (!m_children)
            m_children.emplace(value_tree{});
        tree_row &res = m_children->emplace_back(args...);
        res.m_parent = this;
        return res;
    }

    template <typename ...Args>
    tree_row *addChildPointer(Args&& ...args)
    {
        if (!m_childrenPointers)
            m_childrenPointers.emplace(pointer_tree{});
        auto *res = new tree_row(args...);
        m_childrenPointers->push_back(res);
        res->m_parent = this;
        return res;
    }

    const tree_row *parentRow() const { return m_parent; }
    void setParentRow(tree_row *parent) { m_parent = parent; }
    const std::optional<value_tree> &childRows() const { return m_children; }
    std::optional<value_tree> &childRows() { return m_children; }

    static void prettyPrint(QDebug dbg, const value_tree &tree, int depth = 0)
    {
        dbg.nospace().noquote();
        const QString indent(depth * 2, ' ');
        bool first = true;
        for (const auto &row : tree) {
            dbg << indent;
            if (first && depth) {
                dbg << "\\";
                first = false;
            } else {
                dbg << "|";
            }
            dbg << row << "\n";
            if (const auto &children = row.childRows())
                prettyPrint(dbg, *children, depth + 1);
        }
    }

    struct ProtocolWithChildrenVector {
        tree_row newRow() const { return tree_row{}; }
        void deleteRow(tree_row&& ) { }
        const tree_row *parentRow(const tree_row &row) const { return row.m_parent; }
        void setParentRow(tree_row &row, tree_row *parent) { row.m_parent = parent; }

        const value_tree &childRows(const tree_row &row) const
        {
            return const_cast<ProtocolWithChildrenVector*>(this)->childRows(const_cast<tree_row&>(row));
        }

        value_tree &childRows(tree_row &row)
        {
            if (!row.m_children)
                row.m_children.emplace();
            return *row.m_children;
        }
    };

    struct ProtocolWithChildrenVectorPtr : ProtocolWithChildrenVector {
        const value_tree *childRows(const tree_row &row) const
        {
            return row.m_children ? &*row.m_children : nullptr;
        }

        value_tree *childRows(tree_row &row)
        {
            return &ProtocolWithChildrenVector::childRows(row);
        }
    };

    struct ProtocolPointerImpl {
        tree_row *newRow() const { return new tree_row; }
        void deleteRow(tree_row *row) { delete row; }
        const tree_row *parentRow(const tree_row &row) const { return row.m_parent; }
        void setParentRow(tree_row &row, tree_row *parent) { row.m_parent = parent; }

        const std::optional<pointer_tree> &childRows(const tree_row &row) const
        { return row.m_childrenPointers; }
        std::optional<pointer_tree> &childRows(tree_row &row)
        { return row.m_childrenPointers; }
    };

private:
    QString m_value;
    QString m_description;

    tree_row *m_parent = nullptr;
    std::optional<value_tree> m_children = std::nullopt;
    std::optional<pointer_tree> m_childrenPointers = std::nullopt;

    friend inline QDebug operator<<(QDebug dbg, const tree_row &row)
    {
        QDebugStateSaver saver(dbg);
        dbg.nospace() << row.m_value << " : " << row.m_description;
        if (row.parentRow())
            dbg << " ^ " << row.parentRow()->value();
        if (row.childRows())
            dbg << " v " << row.childRows()->size();
        return dbg;
    }

    template<size_t I, typename Row,
        std::enable_if_t<std::is_same_v<q20::remove_cvref_t<Row>, tree_row>, bool> = true>
    friend inline decltype(auto) get(Row &&row)
    {
        if constexpr (I == 0)
            return row.value();
        else if constexpr (I == 1)
            return row.description();
    }
};

namespace std {
    template <> struct tuple_size<tree_row> : std::integral_constant<size_t, 2> {};
    template <size_t I> struct tuple_element<I, tree_row>
    { using type = decltype(get<I>(std::declval<tree_row>())); };
}

struct Data {

    // fixed number of columns and rows
    std::array<int, 5> fixedArrayOfNumbers = {1, 2, 3, 4, 5};
    int cArrayOfNumbers[5] = {1, 2, 3, 4, 5};
    Row cArrayFixedColumns[3] = {
        {{"red", Qt::red, "0xff0000"}, 0xff0000, "The color red"},
        {{"green", Qt::green, "0x00ff00"}, 0x00ff00, "The color green"},
        {{"blue", Qt::blue, "0x0000ff"}, 0x0000ff, "The color blue"}
    };

    // dynamic number of rows, fixed number of columns
    std::vector<std::tuple<int, QString>> vectorOfFixedColumns = {
        {0, "null"},
        {1, "one"},
        {2, "two"},
        {3, "three"},
        {4, "four"},
    };

    // removable rows, fixed number of columns
    std::vector<std::shared_ptr<std::tuple<int, QString>>> vectorOfFixedSPtrColumns = {
        asSPtr(vectorOfFixedColumns[0]),
        asSPtr(vectorOfFixedColumns[1]),
        asSPtr(vectorOfFixedColumns[2]),
        asSPtr(vectorOfFixedColumns[3]),
        asSPtr(vectorOfFixedColumns[4]),
    };

    std::vector<std::array<int, 10>> vectorOfArrays = {
        {1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
        {11, 12, 13, 14, 15, 16, 17, 18, 19, 20},
        {21, 22, 23, 24, 25, 26, 27, 28, 29, 30},
        {31, 32, 33, 34, 35, 36, 37, 38, 39, 40},
        {41, 42, 43, 44, 45, 46, 47, 48, 49, 50},
    };
    std::vector<Item> vectorOfGadgets = {
        {"red", Qt::red, "0xff0000"},
        {"green", Qt::green, "0x00ff00"},
        {"blue", Qt::blue, "0x0000ff"},
    };
    std::vector<std::tuple<Item>> listOfGadgets = {
        {{"red", Qt::red, "0xff0000"}},
        {{"green", Qt::green, "0x00ff00"}},
        {{"blue", Qt::blue, "0x0000ff"}},
    };
    std::vector<MultiRoleGadget> listOfMultiRoleGadgets = {
        {"red", Qt::red, {}},
        {"green", Qt::green, {}},
        {"blue", Qt::blue, {}},
    };
    std::vector<std::shared_ptr<MultiRoleGadget>> listOfSharedMultiRoleGadgets = {
        asSPtr(MultiRoleGadget{u"red"_s, Qt::red, {}}),
        asSPtr(MultiRoleGadget{u"green"_s, Qt::green, {}}),
        asSPtr(MultiRoleGadget{u"blue"_s, Qt::blue, {}}),
    };

    std::array<std::unique_ptr<MultiRoleGadget>, 3> arrayOfUniqueMultiRoleGadgets = {
        std::make_unique<MultiRoleGadget>(MultiRoleGadget{u"red"_s, Qt::red, {}}),
        std::make_unique<MultiRoleGadget>(MultiRoleGadget{u"green"_s, Qt::green, {}}),
        std::make_unique<MultiRoleGadget>(MultiRoleGadget{u"blue"_s, Qt::blue, {}}),
    };

    std::vector<Row> vectorOfStructs = {
        {{"red", Qt::red, "0xff0000"}, 1, "one"},
        {{"green", Qt::green, "0x00ff00"}, 2, "two"},
        {{"blue", Qt::blue, "0x0000ff"}, 3, "three"},
    };

    std::list<Object *> listOfObjects = {
        new Object, new Object, new Object
    };

    std::array<std::unique_ptr<Object>, 3> arrayOfUniqueObjects = {
        std::make_unique<Object>(), std::make_unique<Object>(), std::make_unique<Object>()
    };

    std::vector<std::tuple<MetaObjectTuple *>> listOfMetaObjectTuple = {
        new MetaObjectTuple,
        new MetaObjectTuple,
        new MetaObjectTuple,
    };
    std::vector<MetaObjectTuple *> tableOfMetaObjectTuple = {
        new MetaObjectTuple,
        new MetaObjectTuple,
        new MetaObjectTuple,
    };
    std::array<std::tuple<std::unique_ptr<MetaObjectTuple>>, 3> arrayOfUniqueMultiObjectTuples {
        std::make_unique<MetaObjectTuple>(),
        std::make_unique<MetaObjectTuple>(),
        std::make_unique<MetaObjectTuple>()
    };

    // bad (but legal) get() overload that never returns a mutable reference
    std::vector<ConstRow> vectorOfConstStructs = {
        {"one"},
        {"two"},
        {"three"},
    };

    // dynamic number of rows and columns
    std::vector<std::vector<double>> tableOfNumbers = {
        {1.0, 2.0, 3.0, 4.0, 5.0},
        {6.0, 7.0, 8.0, 9.0, 10.0},
        {11.0, 12.0, 13.0, 14.0, 15.0},
        {16.0, 17.0, 18.0, 19.0, 20.0},
        {21.0, 22.0, 23.0, 24.0, 25.0},
    };

    // item is pointer
    Item itemAsPointer = {"red", Qt::red, "0xff0000"};
    std::vector<std::vector<Item *>> tableOfPointers = {
        {&itemAsPointer, &itemAsPointer},
        {&itemAsPointer, &itemAsPointer},
        {&itemAsPointer, &itemAsPointer},
    };

    // rows are pointers
    Row rowAsPointer = {{"blue", Qt::blue, "0x0000ff"}, 0x0000ff, "Blau"};
    std::vector<Row *> tableOfRowPointers = {
        &rowAsPointer,
        &rowAsPointer,
        &rowAsPointer,
    };

    // rows are refs
    Row rowAsRef = {{"blue", Qt::blue, "0x0000ff"}, 0x0000ff, "Blau"};
    std::vector<std::reference_wrapper<Row>> tableOfRowRefs = {
        std::ref(rowAsRef),
        std::ref(rowAsRef),
        std::ref(rowAsRef)
    };

    // constness
    std::array<const int, 5> arrayOfConstNumbers = { 1, 2, 3, 4 };
    // note: std::vector doesn't allow for const value types
    const std::vector<int> constListOfNumbers = { 1, 2, 3 };

    // const model is read-only
    const std::vector<std::vector<double>> constTableOfNumbers = {
        {1.0, 2.0, 3.0, 4.0, 5.0},
        {6.0, 7.0, 8.0, 9.0, 10.0},
        {11.0, 12.0, 13.0, 14.0, 15.0},
        {16.0, 17.0, 18.0, 19.0, 20.0},
        {21.0, 22.0, 23.0, 24.0, 25.0},
    };

    // values are associative containers
    std::vector<QVariantMap> listOfNamedRoles = {
        {{"display", "DISPLAY0"}, {"decoration", "DECORATION0"}},
        {{"display", "DISPLAY1"}, {"decoration", "DECORATION1"}},
        {{"display", "DISPLAY2"}, {"decoration", "DECORATION2"}},
        {{"display", "DISPLAY3"}, {"decoration", "DECORATION3"}},
    };
    std::vector<std::vector<std::map<Qt::ItemDataRole, QVariant>>> tableOfEnumRoles = {
        {{{Qt::DisplayRole, "DISPLAY0"}, {Qt::DecorationRole, "DECORATION0"}}},
        {{{Qt::DisplayRole, "DISPLAY1"}, {Qt::DecorationRole, "DECORATION1"}}},
        {{{Qt::DisplayRole, "DISPLAY2"}, {Qt::DecorationRole, "DECORATION2"}}},
        {{{Qt::DisplayRole, "DISPLAY3"}, {Qt::DecorationRole, "DECORATION3"}}},
    };
    std::vector<std::vector<QMap<int, QVariant>>> tableOfIntRoles = {
        {{{Qt::DisplayRole, "DISPLAY0"}, {Qt::DecorationRole, "DECORATION0"}}},
        {{{Qt::DisplayRole, "DISPLAY1"}, {Qt::DecorationRole, "DECORATION1"}}},
        {{{Qt::DisplayRole, "DISPLAY2"}, {Qt::DecorationRole, "DECORATION2"}}},
        {{{Qt::DisplayRole, "DISPLAY3"}, {Qt::DecorationRole, "DECORATION3"}}},
    };

    using VectorOfIntRoleMaps = std::vector<std::map<int, QVariant>>;
    std::vector<VectorOfIntRoleMaps> stdTableOfIntRoles = {
        {{{Qt::DisplayRole, "DISPLAY0"}, {Qt::DecorationRole, "DECORATION0"}}},
        {{{Qt::DisplayRole, "DISPLAY1"}, {Qt::DecorationRole, "DECORATION1"}}},
        {{{Qt::DisplayRole, "DISPLAY2"}, {Qt::DecorationRole, "DECORATION2"}}},
        {{{Qt::DisplayRole, "DISPLAY3"}, {Qt::DecorationRole, "DECORATION3"}}},
    };
    std::vector<std::shared_ptr<VectorOfIntRoleMaps>> stdTableOfIntRolesWithSharedRows = {
        asSPtr(VectorOfIntRoleMaps{{{Qt::DisplayRole, "DISPLAY0"}, {Qt::DecorationRole, "DECORATION0"}}}),
        asSPtr(VectorOfIntRoleMaps{{{Qt::DisplayRole, "DISPLAY1"}, {Qt::DecorationRole, "DECORATION1"}}}),
        asSPtr(VectorOfIntRoleMaps{{{Qt::DisplayRole, "DISPLAY2"}, {Qt::DecorationRole, "DECORATION2"}}}),
        asSPtr(VectorOfIntRoleMaps{{{Qt::DisplayRole, "DISPLAY3"}, {Qt::DecorationRole, "DECORATION3"}}}),
    };

    std::unique_ptr<value_tree> m_tree;
    struct TreeDeleter {
        void operator()(pointer_tree *tree)
        {
            for (auto *row : *tree)
                delete row;
            delete tree;
        }
    };
    std::unique_ptr<pointer_tree, TreeDeleter> m_pointer_tree;
};

#define ADD_HELPER(Model, Tag, Policy, ColumnCount, Actions, HeaderData) \
    { \
        Factory factory = [this]() -> std::unique_ptr<QAbstractItemModel> { \
            auto result = std::make_unique<QRangeModel>(Policy(m_data->Model)); \
            createBackup(result.get(), m_data->Model); \
            return result; \
        }; \
        QTest::addRow(#Model #Tag) << std::move(factory) << int(std::size(m_data->Model)) \
                                   << int(ColumnCount) << ChangeActions(Actions) \
                                   << QVariant::fromValue(HeaderData); \
    }

#define ADD_POINTER(Model, ColumnCount, Actions, HeaderData) \
    ADD_HELPER(Model, Pointer, &, ColumnCount, Actions, HeaderData)
#define ADD_COPY(Model, ColumnCount, Actions, HeaderData) \
    ADD_HELPER(Model, Copy, *&, ColumnCount, Actions, HeaderData)
#define ADD_MOVE(Model, ColumnCount, Actions, HeaderData) \
    ADD_HELPER(Model, Move, std::move, ColumnCount, Actions, HeaderData)
#define ADD_REF(Model, ColumnCount, Actions, HeaderData) \
    ADD_HELPER(Model, Ref, std::ref, ColumnCount, Actions, HeaderData)
#define ADD_UPTR(Model, ColumnCount, Actions, HeaderData) \
    ADD_HELPER(Model, UPtr, asUPtr, ColumnCount, Actions, HeaderData)
#define ADD_SPTR(Model, ColumnCount, Actions, HeaderData) \
    ADD_HELPER(Model, SPtr, asSPtr, ColumnCount, Actions, HeaderData)
#define ADD_ALL(Model, ColumnCount, Actions, HeaderData) \
    ADD_COPY(Model, ColumnCount, Actions, HeaderData) \
    ADD_REF(Model, ColumnCount, Actions, HeaderData) \
    ADD_POINTER(Model, ColumnCount, Actions, HeaderData) \
    ADD_UPTR(Model, ColumnCount, Actions, HeaderData) \
    ADD_SPTR(Model, ColumnCount, Actions, HeaderData)

class QRangeModelTest : public QObject
{
public:
    enum class ChangeAction
    {
        ReadOnly        = 0x00,
        InsertRows      = 0x01,
        RemoveRows      = 0x02,
        ChangeRows      = InsertRows | RemoveRows,
        InsertColumns   = 0x04,
        RemoveColumns   = 0x08,
        ChangeColumns   = InsertColumns | RemoveColumns,
        SetData         = 0x10,
        All             = ChangeRows | ChangeColumns | SetData,
        SetItemData     = 0x20,
    };
    Q_DECLARE_FLAGS(ChangeActions, ChangeAction);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QRangeModelTest::ChangeActions)

// Pointer- and reference-tests will modify the data structure that lives in
// m_data, so we have to keep backup copies of that data.
template <typename T, std::enable_if_t<std::is_copy_assignable_v<T>, bool> = true>
void createBackup(QObject* object, T& model) {
    QObject::connect(object, &QObject::destroyed, object, [backup = model, &model]() mutable {
        model = backup;
    });
}

template <typename T, std::enable_if_t<!std::is_copy_assignable_v<T>, bool> = true>
void createBackup(QObject* , T& ) {}
