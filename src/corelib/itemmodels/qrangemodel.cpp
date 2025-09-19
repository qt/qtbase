// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qrangemodel.h"
#include <QtCore/qsize.h>

#include <QtCore/private/qabstractitemmodel_p.h>

QT_BEGIN_NAMESPACE

class QRangeModelPrivate : QAbstractItemModelPrivate
{
    Q_DECLARE_PUBLIC(QRangeModel)

public:
    explicit QRangeModelPrivate(std::unique_ptr<QRangeModelImplBase, QRangeModelImplBase::Deleter> impl)
        : impl(std::move(impl))
    {}

private:
    std::unique_ptr<QRangeModelImplBase, QRangeModelImplBase::Deleter> impl;
    mutable QHash<int, QByteArray> m_roleNames;
};

QRangeModel::QRangeModel(QRangeModelImplBase *impl, QObject *parent)
    : QAbstractItemModel(*new QRangeModelPrivate({impl, {}}), parent)
{
}

/*!
    \class QRangeModel
    \inmodule QtCore
    \since 6.10
    \ingroup model-view
    \brief QRangeModel implements QAbstractItemModel for any C++ range.
    \reentrant

    QRangeModel can make the data in any sequentially iterable C++ type
    available to the \l{Model/View Programming}{model/view framework} of Qt.
    This makes it easy to display existing data structures in the Qt Widgets
    and Qt Quick item views, and to allow the user of the application to
    manipulate the data using a graphical user interface.

    To use QRangeModel, instantiate it with a C++ range and set it as
    the model of one or more views:

    \snippet qrangemodel/main.cpp array

    \section1 Constructing the model

    The range can be any C++ type for which the standard methods
    \c{std::begin} and \c{std::end} are implemented, and for which the
    returned iterator type satisfies \c{std::forward_iterator}. Certain model
    operations will perform better if \c{std::size} is available, and if the
    iterator satisfies \c{std::random_access_iterator}.

    The range must be provided when constructing the model; there is no API to
    set the range later, and there is no API to retrieve the range from the
    model. The range can be provided by value, reference wrapper, or pointer.
    How the model was constructed defines whether changes through the model API
    will modify the original data.

    When constructed by value, the model makes a copy of the range, and
    QAbstractItemModel APIs that modify the model, such as setData() or
    insertRows(), have no impact on the original range.

    \snippet qrangemodel/main.cpp value

    As there is no API to retrieve the range again, constructing the model from
    a range by value is mostly only useful for displaying read-only data.
    Changes to the data can be monitored using the signals emitted by the
    model, such as \l{QAbstractItemModel}{dataChanged()}.

    To make modifications of the model affect the original range, provide the
    range either by pointer:

    \snippet qrangemodel/main.cpp pointer

    or through a reference wrapper:

    \snippet qrangemodel/main.cpp reference_wrapper

    In this case, QAbstractItemModel APIs that modify the model also modify the
    range. Methods that modify the structure of the range, such as insertRows()
    or removeColumns(), use standard C++ container APIs \c{resize()},
    \c{insert()}, \c{erase()}, in addition to dereferencing a mutating iterator
    to set or clear the data.

    \note Once the model has been constructed and passed on to a view, the
    range that the model operates on must no longer be modified directly. Views
    on the model wouldn't be informed about the changes, and structural changes
    are likely to corrupt instances of QPersistentModelIndex that the model
    maintains.

    The caller must make sure that the range's lifetime exceeds the lifetime of
    the model.

    Use smart pointers to make sure that the range is only deleted when all
    clients are done with it.

    \snippet qrangemodel/main.cpp smart_pointer

    QRangeModel supports both shared and unique pointers.

    \section2 Read-only or mutable

    For ranges that are const objects, for which access always yields constant
    values, or where the required container APIs are not available,
    QRangeModel implements write-access APIs to do nothing and return
    \c{false}. In the example using \c{std::array}, the model cannot add or
    remove rows, as the number of entries in a C++ array is fixed. But the
    values can be changed using setData(), and the user can trigger editing of
    the values in the list view. By making the array const, the values also
    become read-only.

    \snippet qrangemodel/main.cpp const_array

    The values are also read-only if the element type is const, like in

    \snippet qrangemodel/main.cpp const_values

    In the above examples using \c{std::vector}, the model can add or remove
    rows, and the data can be changed. Passing the range as a constant
    reference will make the model read-only.

    \snippet qrangemodel/main.cpp const_ref

    \note If the values in the range are const, then it's also not possible
    to remove or insert columns and rows through the QAbstractItemModel API.
    For more granular control, implement \l{the C++ tuple protocol}.

    \section1 Rows and columns

    The elements in the range are interpreted as rows of the model. Depending
    on the type of these row elements, QRangeModel exposes the range as a
    list, a table, or a tree.

    If the row elements are simple values, then the range gets represented as a
    list.

    \snippet qrangemodel/main.cpp list_of_int

    If the type of the row elements is an iterable range, such as a vector,
    list, or array, then the range gets represented as a table.

    \snippet qrangemodel/main.cpp grid_of_numbers

    If the row type provides the standard C++ container APIs \c{resize()},
    \c{insert()}, \c{erase()}, then columns can be added and removed via
    insertColumns() and removeColumns(). All rows are required to have
    the same number of columns.

    \section2 Structs and gadgets as rows

    If the row type implements \l{the C++ tuple protocol}, then the range gets
    represented as a table with a fixed number of columns.

    \snippet qrangemodel/main.cpp pair_int_QString

    An easier and more flexible alternative to implementing the tuple protocol
    for a C++ type is to use Qt's \l{Meta-Object System}{meta-object system} to
    declare a type with \l{Qt's Property System}{properties}. This can be a
    value type that is declared as a \l{Q_GADGET}{gadget}, or a QObject subclass.

    \snippet qrangemodel/main.cpp gadget

    Using QObject subclasses allows properties to be \l{Qt Bindable Properties}
    {bindable}, or to have change notification signals. However, using QObject
    instances for items has significant memory overhead.

    Using Qt gadgets or objects is more convenient and can be more flexible
    than implementing the tuple protocol. Those types are also directly
    accessible from within QML. However, the access through \l{the property system}
    comes with some runtime overhead. For performance critical models, consider
    implementing the tuple protocol for compile-time generation of the access
    code.

    \section2 Multi-role items

    The type of the items that the implementations of data(), setData(),
    clearItemData() etc. operate on can be the same across the entire model -
    like in the \c{gridOfNumbers} example above. But the range can also have
    different item types for different columns, like in the \c{numberNames}
    case.

    By default, the value gets used for the Qt::DisplayRole and Qt::EditRole
    roles. Most views expect the value to be
    \l{QVariant::canConvert}{convertible to and from a QString} (but a custom
    delegate might provide more flexibility).

    \section3 Associative containers with multiple roles

    If the item is an associative container that uses \c{int},
    \l{Qt::ItemDataRole}, or QString as the key type, and QVariant as the
    mapped type, then QRangeModel interprets that container as the storage
    of the data for multiple roles. The data() and setData() functions return
    and modify the mapped value in the container, and setItemData() modifies all
    provided values, itemData() returns all stored values, and clearItemData()
    clears the entire container.

    \snippet qrangemodel/main.cpp color_map

    The most efficient data type to use as the key is Qt::ItemDataRole or
    \c{int}. When using \c{int}, itemData() returns the container as is, and
    doesn't have to create a copy of the data.

    \section3 Gadgets and Objects as multi-role items

    Gadgets and QObject types can also be represented as multi-role items. The
    \l{The Property System}{properties} of those items will be used for the
    role for which the \l{roleNames()}{name of a role} matches. If all items
    hold the same type of gadget or QObject, then the \l{roleNames()}
    implementation in QRangeModel will return the list of properties of that
    type.

    \snippet qrangemodel/main.cpp color_gadget_decl
    \snippet qrangemodel/main.cpp color_gadget_impl
    \snippet qrangemodel/main.cpp color_gadget_end

    When used in a table, this is the default representation for gadgets:

    \snippet qrangemodel/main.cpp color_gadget_table

    When used in a list, these types are however by default represented as
    multi-column rows, with each property represented as a separate column. To
    force a gadget to be represented as a multi-role item in a list, declare
    the gadget as a multi-role type by specializing QRoleModel::RowOptions,
    with a \c{static constexpr auto rowCategory} member variable set to
    MultiRoleItem.

    \snippet qrangemodel/main.cpp color_gadget_decl
    \dots
    \snippet qrangemodel/main.cpp color_gadget_end
    \snippet qrangemodel/main.cpp color_gadget_multi_role_gadget

    You can also wrap such types into a single-element tuple, turning the list
    into a table with a single column:

    \snippet qrangemodel/main.cpp color_gadget_single_column

    In this case, note that direct access to the elements in the list data
    needs to use \c{std::get}:

    \snippet qrangemodel/main.cpp color_gadget_single_column_access_get

    or alternatively a structured binding:

    \snippet qrangemodel/main.cpp color_gadget_single_column_access_sb

    \section2 Rows as values or pointers

    In the examples so far, we have always used QRangeModel with ranges that
    hold values. QRangeModel can also operate on ranges that hold pointers,
    including smart pointers. This allows QRangeModel to operate on ranges of
    polymorph types, such as QObject subclasses.

    \snippet qrangemodel/main.cpp object_0
    \dots
    \snippet qrangemodel/main.cpp object_1

    \snippet qrangemodel/main.cpp vector_of_objects_0
    \dots
    \snippet qrangemodel/main.cpp vector_of_objects_1
    \snippet qrangemodel/main.cpp vector_of_objects_2

    As with values, the type of the row defines whether the range is
    represented as a list, table, or tree. Rows that are QObjects will present
    each property as a column, unless the QRangeModel::RowOptions template is
    specialized to declare the type as a multi-role item.

    \snippet qrangemodel/main.cpp vector_of_multirole_objects_0
    \snippet qrangemodel/main.cpp vector_of_multirole_objects_1
    \dots
    \snippet qrangemodel/main.cpp vector_of_multirole_objects_2

    \note If the range holds raw pointers, then you have to construct
    QRangeModel from a pointer or reference wrapper of the range. Otherwise the
    ownership of the data becomes ambiguous, and a copy of the range would
    still be operating on the same actual row data, resulting in unexpected
    side effects.

    \section2 Subclassing QRangeModel

    Subclassing QRangeModel makes it possible to add convenient APIs that take
    the data type and structure of the range into account.

    \snippet qrangemodel/main.cpp subclass_header

    When doing so, add the range as a private member, and call the QRangeModel
    constructor with a reference wrapper or pointer to that member. This
    properly encapsulates the data and avoids direct access.

    \snippet qrangemodel/main.cpp subclass_API

    Add member functions to provide type-safe access to the data, using the
    QAbstractItemModel API to perform any operation that modifies the range.
    Read-only access can directly operate on the data structure.

    \section1 Trees of data

    QRangeModel can represent a data structure as a tree model. Such a
    tree data structure needs to be homomorphic: on all levels of the tree, the
    list of child rows needs to use the exact same representation as the tree
    itself. In addition, the row type needs be of a static size: either a gadget
    or QObject type, or a type that implements the {C++ tuple protocol}.

    To represent such data as a tree, QRangeModel has to be able to traverse the
    data structure: for any given row, the model needs to be able to retrieve
    the parent row, and the optional span of children. These traversal functions
    can be provided implicitly through the row type, or through an explicit
    protocol type.

    \section2 Implicit tree traversal protocol

    \snippet qrangemodel/main.cpp tree_protocol_0

    The tree itself is a vector of \c{TreeRow} values. See \l{Tree Rows as
    pointers or values} for the considerations on whether to use values or
    pointers of items for the rows.

    \snippet qrangemodel/main.cpp tree_protocol_1

    The row class can be of any fixed-size type described above: a type that
    implements the tuple protocol, a gadget, or a QObject. In this example, we
    use a gadget.

    Each row item needs to maintain a pointer to the parent row, as well as an
    optional range of child rows. That range has to be identical to the range
    structure used for the tree itself.

    Making the row type default constructible is optional, and allows the model
    to construct new row data elements, for instance in the insertRow() or
    moveRows() implementations.

    \snippet qrangemodel/main.cpp tree_protocol_2

    The tree traversal protocol can then be implemented as member functions of
    the row data type. A const \c{parentRow()} function has to return a pointer
    to a const row item; and the \c{childRows()} function has to return a
    reference to a const \c{std::optional} that can hold the optional child
    range.

    These two functions are sufficient for the model to navigate the tree as a
    read-only data structure. To allow the user to edit data in a view, and the
    model to implement mutating model APIs such as insertRows(), removeRows(),
    and moveRows(), we have to implement additional functions for write-access:

    \snippet qrangemodel/main.cpp tree_protocol_3

    The model calls the \c{setParentRow()} function and mutable \c{childRows()}
    overload to move or insert rows into an existing tree branch, and to update
    the parent pointer should the old value have become invalid. The non-const
    overload of \c{childRows()} provides in addition write-access to the row
    data.

    \note The model performs setting the parent of a row, removing that row
    from the old parent, and adding it to the list of the new parent's children,
    as separate steps. This keeps the protocol interface small.

    \dots
    \snippet qrangemodel/main.cpp tree_protocol_4

    The rest of the class implementation is not relevant for the model, but
    a \c{addChild()} helper provides us with a convenient way to construct the
    initial state of the tree.

    \snippet qrangemodel/main.cpp tree_protocol_5

    A QRangeModel instantiated with an instance of such a range will
    represent the data as a tree.

    \snippet qrangemodel/main.cpp tree_protocol_6

    \section2 Tree traversal protocol in a separate class

    The tree traversal protocol can also be implemented in a separate class.

    \snippet qrangemodel/main.cpp explicit_tree_protocol_0

    Pass an instance of this protocol implementation to the QRangeModel
    constructor:

    \snippet qrangemodel/main.cpp explicit_tree_protocol_1

    \section2 Tree Rows as pointers or values

    The row type of the data range can be either a value, or a pointer. In
    the code above we have been using the tree rows as values in a vector,
    which avoids that we have to deal with explicit memory management. However,
    a vector as a contiguous block of memory invalidates all iterators and
    references when it has to reallocate the storage, or when inserting or
    removing elements. This impacts the pointer to the parent item, which is
    the location of the parent row within the vector. Making sure that this
    parent (and QPersistentModelIndex instances referring to items within it)
    stays valid can incurr substantial performance overhead. The
    QRangeModel implementation has to assume that all references into the
    range become invalid when modifying the range.

    Alternatively, we can also use a range of row pointers as the tree type:

    \snippet qrangemodel/main.cpp tree_of_pointers_0

    In this case, we have to allocate all TreeRow instances explicitly using
    operator \c{new}, and implement the destructor to \c{delete} all items in
    the vector of children.

    \snippet qrangemodel/main.cpp tree_of_pointers_1
    \snippet qrangemodel/main.cpp tree_of_pointers_2

    Before we can construct a model that represents this data as a tree, we need
    to also implement the tree traversal protocol.

    \snippet qrangemodel/main.cpp tree_of_pointers_3

    An explicit protocol implementation for mutable trees of pointers has to
    provide two additional member functions, \c{newRow()} and
    \c{deleteRow(RowType *)}.

    \snippet qrangemodel/main.cpp tree_of_pointers_4

    The model will call those functions when creating new rows in insertRows(),
    and when removing rows in removeRows(). In addition, if the model has
    ownership of the data, then it will also delete all top-level rows upon
    destruction. Note how in this example, we move the tree into the model, so
    we must no longer perform any operations on it. QRangeModel, when
    constructed by moving tree-data with row-pointers into it, will take
    ownership of the data, and delete the row pointers in it's destructor.

    Using pointers as rows comes with some memory allocation and management
    overhead. However, the references to the row items remain stable, even when
    they are moved around in the range, or when the range reallocates. This can
    significantly reduce the cost of making modifications to the model's
    structure when using insertRows(), removeRows(), or moveRows().

    Each choice has different performance and memory overhead trade-offs. The
    best option depends on the exact use case and data structure used.

    \section2 The C++ tuple protocol

    As seen in the \c{numberNames} example above, the row type can be a tuple,
    and in fact any type that implements the tuple protocol. This protocol is
    implemented by specializing \c{std::tuple_size} and \c{std::tuple_element},
    and overloading the unqualified \c{get} function. Do so for your custom row
    type to make existing structured data available to the model/view framework
    in Qt.

    \snippet qrangemodel/main.cpp tuple_protocol

    In the above implementation, the \c{title} and \c{author} values of the
    \c{Book} type are returned as \c{const}, so the model flags items in those
    two columns as read-only. The user won't be able to trigger editing, and
    setData() does nothing and returns false. For \c{summary} and \c{rating}
    the implementation returns the same value category as the book, so when
    \c{get} is called with a mutable reference to a \c{Book}, then it will
    return a mutable reference of the respective variable. The model makes
    those columns editable, both for the user and for programmatic access.

    \note The implementation of \c{get} above requires C++23.

    \sa {Model/View Programming}
*/

/*!
    \class QRangeModel::RowOptions
    \inmodule QtCore
    \ingroup model-view
    \brief The RowOptions template provides a customization point to control
           how QRangeModel represents types used as rows.
    \since 6.10

    Specialize this template for the type used in your range, and add the
    relevant members.

    \table
    \header
        \li Member
        \li Values
    \row
        \li static constexpr RowCategory rowCategory
        \li RowCategory
    \endtable

    \snippet qrangemodel/main.cpp color_gadget_decl
    \dots
    \snippet qrangemodel/main.cpp color_gadget_end
    \snippet qrangemodel/main.cpp color_gadget_multi_role_gadget

*/

/*!
    \enum QRangeModel::RowCategory

    This enum describes how QRangeModel should present the elements of the
    range it was constructed with.

    \value Default
           QRangeModel decides how to present the rows.
    \value MultiRoleItem
           QRangeModel will present items with a meta object as multi-role
           items, also when used in a one-dimensional range.

    Specialize the RowOptions template for your type, and add a public member
    variable \c{static constexpr auto rowCategory} with one of the values from
    this enum.

    \sa RowOptions
*/

/*!
    \fn template <typename Range, QRangeModelDetails::if_table_range<Range>> QRangeModel::QRangeModel(Range &&range, QObject *parent)
    \fn template <typename Range, QRangeModelDetails::if_tree_range<Range>> QRangeModel::QRangeModel(Range &&range, QObject *parent)
    \fn template <typename Range, typename Protocol, QRangeModelDetails::if_tree_range<Range, Protocol>> QRangeModel::QRangeModel(Range &&range, Protocol &&protocol, QObject *parent)

    Constructs a QRangeModel instance that operates on the data in \a range.
    The \a range has to be a sequential range for which \c{std::begin} and
    \c{std::end} are available. If \a protocol is provided, then the model
    will represent the range as a tree using the protocol implementation. The
    model instance becomes a child of \a parent.

    The \a range can be a pointer or reference wrapper, in which case mutating
    model APIs (such as \l{setData()} or \l{insertRow()}) will modify the data
    in the referenced range instance. If \a range is a value (or moved into the
    model), then connect to the signals emitted by the model to respond to
    changes to the data.

    QRangeModel will not access the \a range while being constructed. This
    makes it legal to pass a pointer or reference to a range object that is not
    fully constructed yet to this constructor, for example when \l{Subclassing
    QRangeModel}{subclassing QRangeModel}.

    If the \a range was moved into the model, then the range and all data in it
    will be destroyed upon destruction of the model.

    \note While the model does not take ownership of the range object otherwise,
    you must not modify the \a range directly once the model has been constructed
    and  and passed on to a view. Such modifications will not emit signals
    necessary to keep model users (other models or views) synchronized with the
    model, resulting in inconsistent results, undefined behavior, and crashes.
*/

/*!
    Destroys the QRangeModel.

    The range that the model was constructed from is not accessed, and only
    destroyed if the model was constructed from a moved-in range.
*/
QRangeModel::~QRangeModel() = default;

/*!
    \reimp

    Returns the index of the model item at \a row and \a column in \a parent.

    Passing a valid parent produces an invalid index for models that operate on
    list and table ranges.

    \sa parent()
*/
QModelIndex QRangeModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_D(const QRangeModel);
    return d->impl->call<QRangeModelImplBase::Index>(row, column, parent);
}

/*!
    \reimp

    Returns the parent of the item at the \a child index.

    This function always produces an invalid index for models that operate on
    list and table ranges. For models operation on a tree, this function
    returns the index for the row item returned by the parent() implementation
    of the tree traversal protocol.

    \sa index(), hasChildren()
*/
QModelIndex QRangeModel::parent(const QModelIndex &child) const
{
    Q_D(const QRangeModel);
    return d->impl->call<QRangeModelImplBase::Parent>(child);
}

/*!
    \reimp

    Returns the sibling at \a row and \a column for the item at \a index, or an
    invalid QModelIndex if there is no sibling at that location.

    This implementation is significantly faster than going through the parent()
    of the \a index.

    \sa index(), QModelIndex::row(), QModelIndex::column()
*/
QModelIndex QRangeModel::sibling(int row, int column, const QModelIndex &index) const
{
    Q_D(const QRangeModel);
    return d->impl->call<QRangeModelImplBase::Sibling>(row, column, index);
}

/*!
    \reimp

    Returns the number of rows under the given \a parent. This is the number of
    items in the root range for an invalid \a parent index.

    If the \a parent index is valid, then this function always returns 0 for
    models that operate on list and table ranges. For trees, this returns the
    size of the range returned by the childRows() implementation of the tree
    traversal protocol.

    \sa columnCount(), insertRows(), hasChildren()
*/
int QRangeModel::rowCount(const QModelIndex &parent) const
{
    Q_D(const QRangeModel);
    return d->impl->call<QRangeModelImplBase::RowCount>(parent);
}

/*!
    \reimp

    Returns the number of columns of the model. This function returns the same
    value for all \a parent indexes.

    For models operating on a statically sized row type, this returned value is
    always the same throughout the lifetime of the model. For models operating
    on dynamically sized row type, the model returns the number of items in the
    first row, or 0 if the model has no rows.

    \sa rowCount, insertColumns()
*/
int QRangeModel::columnCount(const QModelIndex &parent) const
{
    Q_D(const QRangeModel);
    return d->impl->call<QRangeModelImplBase::ColumnCount>(parent);
}

/*!
    \reimp

    Returns the item flags for the given \a index.

    The implementation returns a combination of flags that enables the item
    (\c ItemIsEnabled) and allows it to be selected (\c ItemIsSelectable). For
    models operating on a range with mutable data, it also sets the flag
    that allows the item to be editable (\c ItemIsEditable).

    \sa Qt::ItemFlags
*/
Qt::ItemFlags QRangeModel::flags(const QModelIndex &index) const
{
    Q_D(const QRangeModel);
    return d->impl->call<QRangeModelImplBase::Flags>(index);
}

/*!
    \reimp

    Returns the data for the given \a role and \a section in the header with
    the specified \a orientation.

    For horizontal headers, the section number corresponds to the column
    number. Similarly, for vertical headers, the section number corresponds to
    the row number.

    For the horizontal header and the Qt::DisplayRole \a role, models that
    operate on a range that uses an array as the row type return \a section. If
    the row type is a tuple, then the implementation returns the name of the
    type at \a section. For rows that are a gadget or QObject type, this
    function returns the name of the property at the index of \a section.

    For the vertical header, this function always returns the result of the
    default implementation in QAbstractItemModel.

    \sa Qt::ItemDataRole, setHeaderData(), QHeaderView
*/
QVariant QRangeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_D(const QRangeModel);
    return d->impl->call<QRangeModelImplBase::HeaderData>(section, orientation, role);
}

/*!
    \reimp
*/
bool QRangeModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant &data,
                                int role)
{
    return QAbstractItemModel::setHeaderData(section, orientation, data, role);
}

/*!
    \reimp

    Returns the data stored under the given \a role for the value in the
    range referred to by the \a index.

    If the item type for that index is an associative container that maps from
    either \c{int}, Qt::ItemDataRole, or QString to a QVariant, then the role
    data is looked up in that container and returned.

    If the item is a gadget or QObject, then the implementation returns the
    value of the item's property matching the \a role entry in the roleNames()
    mapping.

    Otherwise, the implementation returns a QVariant constructed from the item
    via \c{QVariant::fromValue()} for \c{Qt::DisplayRole} or \c{Qt::EditRole}.
    For other roles, the implementation returns an \b invalid
    (default-constructed) QVariant.

    \sa Qt::ItemDataRole, setData(), headerData()
*/
QVariant QRangeModel::data(const QModelIndex &index, int role) const
{
    Q_D(const QRangeModel);
    return d->impl->call<QRangeModelImplBase::Data>(index, role);
}

/*!
    \reimp

    Sets the \a role data for the item at \a index to \a data.

    If the item type for that \a index is an associative container that maps
    from either \c{int}, Qt::ItemDataRole, or QString to a QVariant, then
    \a data is stored in that container for the key specified by \a role.

    If the item is a gadget or QObject, then \a data is written to the item's
    property matching the \a role entry in the the roleNames() mapping. The
    function returns \c{true} if a property was found and if \a data stored a
    value that could be converted to the required type, otherwise returns
    \c{false}.

    Otherwise, this implementation assigns the value in \a data to the item at
    the \a index in the range for \c{Qt::DisplayRole} and \c{Qt::EditRole},
    and returns \c{true}. For other roles, the implementation returns
    \c{false}.

//! [read-only-setData]
    For models operating on a read-only range, or on a read-only column in
    a row type that implements \l{the C++ tuple protocol}, this implementation
    returns \c{false} immediately.
//! [read-only-setData]
*/
bool QRangeModel::setData(const QModelIndex &index, const QVariant &data, int role)
{
    Q_D(QRangeModel);
    return d->impl->call<QRangeModelImplBase::SetData>(index, data, role);
}

/*!
    \reimp

    Returns a map with values for all predefined roles in the model for the
    item at the given \a index.

    If the item type for that \a index is an associative container that maps
    from either \c{int}, Qt::ItemDataRole, or QString to a QVariant, then the
    data from that container is returned.

    If the item type is a gadget or QObject subclass, then the values of those
    properties that match a \l{roleNames()}{role name} are returned.

    If the item is not an associative container, gadget, or QObject subclass,
    then this calls the base class implementation.

    \sa setItemData(), Qt::ItemDataRole, data()
*/
QMap<int, QVariant> QRangeModel::itemData(const QModelIndex &index) const
{
    Q_D(const QRangeModel);
    return d->impl->call<QRangeModelImplBase::ItemData>(index);
}

/*!
    \reimp

    If the item type for that \a index is an associative container that maps
    from either \c{int} or Qt::ItemDataRole to a QVariant, then the entries in
    \a data are stored in that container. If the associative container maps from
    QString to QVariant, then only those values in \a data are stored for which
    there is a mapping in the \l{roleNames()}{role names} table.

    If the item type is a gadget or QObject subclass, then those properties that
    match a \l{roleNames()}{role name} are set to the corresponding value in
    \a data.

    Roles for which there is no entry in \a data are not modified.

    For item types that can be copied, this implementation is transactional,
    and returns true if all the entries from \a data could be stored. If any
    entry could not be updated, then the original container is not modified at
    all, and the function returns false.

    If the item is not an associative container, gadget, or QObject subclass,
    then this calls the base class implementation, which calls setData() for
    each entry in \a data.

    \sa itemData(), setData(), Qt::ItemDataRole
*/
bool QRangeModel::setItemData(const QModelIndex &index, const QMap<int, QVariant> &data)
{
    Q_D(QRangeModel);
    return d->impl->call<QRangeModelImplBase::SetItemData>(index, data);
}

/*!
    \reimp

    Replaces the value stored in the range at \a index with a default-
    constructed value.

    \include qrangemodel.cpp read-only-setData
*/
bool QRangeModel::clearItemData(const QModelIndex &index)
{
    Q_D(QRangeModel);
    return d->impl->call<QRangeModelImplBase::ClearItemData>(index);
}

/*
//! [column-change-requirement]
    \note A dynamically sized row type needs to provide a \c{\1} member function.

    For models operating on a read-only range, or on a range with a
    statically sized row type (such as a tuple, array, or struct), this
    implementation does nothing and returns \c{false} immediately. This is
    always the case for tree models.
//! [column-change-requirement]
*/

/*!
    \reimp

    Inserts \a count empty columns before the item at \a column in all rows
    of the range at \a parent. Returns \c{true} if successful; otherwise
    returns \c{false}.

    \include qrangemodel.cpp {column-change-requirement} {insert(const_iterator, size_t, value_type)}
*/
bool QRangeModel::insertColumns(int column, int count, const QModelIndex &parent)
{
    Q_D(QRangeModel);
    return d->impl->call<QRangeModelImplBase::InsertColumns>(column, count, parent);
}

/*!
    \reimp

    Removes \a count columns from the item at \a column on in all rows of the
    range at \a parent. Returns \c{true} if successful, otherwise returns
    \c{false}.

    \include qrangemodel.cpp {column-change-requirement} {erase(const_iterator, size_t)}
*/
bool QRangeModel::removeColumns(int column, int count, const QModelIndex &parent)
{
    Q_D(QRangeModel);
    return d->impl->call<QRangeModelImplBase::RemoveColumns>(column, count, parent);
}

/*!
    \reimp

    Moves \a count columns starting with the given \a sourceColumn under parent
    \a sourceParent to column \a destinationColumn under parent \a destinationParent.

    Returns \c{true} if the columns were successfully moved; otherwise returns
    \c{false}.
*/
bool QRangeModel::moveColumns(const QModelIndex &sourceParent, int sourceColumn, int count,
                                    const QModelIndex &destinationParent, int destinationColumn)
{
    Q_D(QRangeModel);
    return d->impl->call<QRangeModelImplBase::MoveColumns>(
                         sourceParent, sourceColumn, count,
                         destinationParent, destinationColumn);
}

/*
//! [row-change-requirement]
    \note The range needs to be dynamically sized and provide a \c{\1}
    member function.

    For models operating on a read-only or statically-sized range (such as
    an array), this implementation does nothing and returns \c{false}
    immediately.
//! [row-change-requirement]
*/

/*!
    \reimp

    Inserts \a count empty rows before the given \a row into the range at
    \a parent. Returns \c{true} if successful; otherwise returns \c{false}.

    \include qrangemodel.cpp {row-change-requirement} {insert(const_iterator, size_t, value_type)}

    \note For ranges with a dynamically sized column type, the column needs
    to provide a \c{resize(size_t)} member function.
*/
bool QRangeModel::insertRows(int row, int count, const QModelIndex &parent)
{
    Q_D(QRangeModel);
    return d->impl->call<QRangeModelImplBase::InsertRows>(row, count, parent);
}

/*!
    \reimp

    Removes \a count rows from the range at \a parent, starting with the
    given \a row. Returns \c{true} if successful, otherwise returns \c{false}.

    \include qrangemodel.cpp {row-change-requirement} {erase(const_iterator, size_t)}
*/
bool QRangeModel::removeRows(int row, int count, const QModelIndex &parent)
{
    Q_D(QRangeModel);
    return d->impl->call<QRangeModelImplBase::RemoveRows>(row, count, parent);
}

/*!
    \reimp

    Moves \a count rows starting with the given \a sourceRow under parent
    \a sourceParent to row \a destinationRow under parent \a destinationParent.

    Returns \c{true} if the rows were successfully moved; otherwise returns
    \c{false}.
*/
bool QRangeModel::moveRows(const QModelIndex &sourceParent, int sourceRow, int count,
                                 const QModelIndex &destinationParent, int destinationRow)
{
    Q_D(QRangeModel);
    return d->impl->call<QRangeModelImplBase::MoveRows>(
                         sourceParent, sourceRow, count,
                         destinationParent, destinationRow);
}

/*!
    \reimp
*/
bool QRangeModel::canFetchMore(const QModelIndex &parent) const
{
    return QAbstractItemModel::canFetchMore(parent);
}

/*!
    \reimp
*/
void QRangeModel::fetchMore(const QModelIndex &parent)
{
    QAbstractItemModel::fetchMore(parent);
}

/*!
    \reimp
*/
bool QRangeModel::hasChildren(const QModelIndex &parent) const
{
    return QAbstractItemModel::hasChildren(parent);
}

/*!
    \reimp
*/
QModelIndex QRangeModel::buddy(const QModelIndex &index) const
{
    return QAbstractItemModel::buddy(index);
}

/*!
    \reimp
*/
bool QRangeModel::canDropMimeData(const QMimeData *data, Qt::DropAction action,
                                        int row, int column, const QModelIndex &parent) const
{
    return QAbstractItemModel::canDropMimeData(data, action, row, column, parent);
}

/*!
    \reimp
*/
bool QRangeModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
                                     int row, int column, const QModelIndex &parent)
{
    return QAbstractItemModel::dropMimeData(data, action, row, column, parent);
}

/*!
    \reimp
*/
QMimeData *QRangeModel::mimeData(const QModelIndexList &indexes) const
{
    return QAbstractItemModel::mimeData(indexes);
}

/*!
    \reimp
*/
QStringList QRangeModel::mimeTypes() const
{
    return QAbstractItemModel::mimeTypes();
}

/*!
    \reimp
*/
QModelIndexList QRangeModel::match(const QModelIndex &start, int role, const QVariant &value,
                                         int hits, Qt::MatchFlags flags) const
{
    return QAbstractItemModel::match(start, role, value, hits, flags);
}

/*!
    \reimp
*/
void QRangeModel::multiData(const QModelIndex &index, QModelRoleDataSpan roleDataSpan) const
{
    QAbstractItemModel::multiData(index, roleDataSpan);
}


/*!
    \property QRangeModel::roleNames
    \brief the role names for the model.

    If all columns in the range are of the same type, and if that type provides
    a meta object (i.e., it is a gadget, or a QObject subclass), then this
    property holds the names of the properties of that type, mapped to values of
    Qt::ItemDataRole values from Qt::UserRole and up. In addition, a role
    "modelData" provides access to the gadget or QObject instance.

    Override this default behavior by setting this property explicitly to a non-
    empty mapping. Setting this property to an empty mapping, or using
    resetRoleNames(), restores the default behavior.

    \sa QAbstractItemModel::roleNames()
*/

QHash<int, QByteArray> QRangeModelImplBase::roleNamesForMetaObject(const QAbstractItemModel &model,
                                                                   const QMetaObject &metaObject)
{
    const auto defaults = model.QAbstractItemModel::roleNames();
    QHash<int, QByteArray> result = {{Qt::RangeModelDataRole, "modelData"}};
    int offset = metaObject.propertyOffset();
    for (int i = offset; i < metaObject.propertyCount(); ++i) {
        const auto name = metaObject.property(i).name();
        const int defaultRole = defaults.key(name, -1);
        if (defaultRole != -1) {
            ++offset;
            result[defaultRole] = name;
        } else {
            result[Qt::UserRole + i - offset] = name;
        }
    }
    return result;
}

QHash<int, QByteArray> QRangeModelImplBase::roleNamesForSimpleType()
{
    // just a plain value
    return QHash<int, QByteArray>{
        {Qt::DisplayRole, "display"},
        {Qt::EditRole, "edit"},
        {Qt::RangeModelDataRole, "modelData"},
    };
}

/*!
    \reimp

    \note Overriding this function in a QRangeModel subclass is possible,
    but might break the behavior of the property.
*/
QHash<int, QByteArray> QRangeModel::roleNames() const
{
    Q_D(const QRangeModel);
    if (d->m_roleNames.isEmpty())
        d->m_roleNames = d->impl->call<QRangeModelImplBase::RoleNames>();

    return d->m_roleNames;
}

void QRangeModel::setRoleNames(const QHash<int, QByteArray> &names)
{
    Q_D(QRangeModel);
    if (d->m_roleNames == names)
        return;
    beginResetModel();
    d->impl->call<QRangeModelImplBase::InvalidateCaches>();
    d->m_roleNames = names;
    endResetModel();
    Q_EMIT roleNamesChanged();
}

void QRangeModel::resetRoleNames()
{
    setRoleNames({});
}

/*!
    \reimp
*/
void QRangeModel::sort(int column, Qt::SortOrder order)
{
    return QAbstractItemModel::sort(column, order);
}

/*!
    \reimp
*/
QSize QRangeModel::span(const QModelIndex &index) const
{
    return QAbstractItemModel::span(index);
}

/*!
    \reimp
*/
Qt::DropActions QRangeModel::supportedDragActions() const
{
    return QAbstractItemModel::supportedDragActions();
}

/*!
    \reimp
*/
Qt::DropActions QRangeModel::supportedDropActions() const
{
    return QAbstractItemModel::supportedDropActions();
}

/*!
    \reimp
*/
void QRangeModel::resetInternalData()
{
    QAbstractItemModel::resetInternalData();
}

/*!
    \reimp
*/
bool QRangeModel::event(QEvent *event)
{
    return QAbstractItemModel::event(event);
}

/*!
    \reimp
*/
bool QRangeModel::eventFilter(QObject *object, QEvent *event)
{
    return QAbstractItemModel::eventFilter(object, event);
}

QT_END_NAMESPACE

#include "moc_qrangemodel.cpp"
