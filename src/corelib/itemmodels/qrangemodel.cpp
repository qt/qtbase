// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qrangemodel.h"
#include <QtCore/qcollator.h>
#include <QtCore/qmimedata.h>
#include <QtCore/qpoint.h>
#include <QtCore/qsize.h>

#include <QtCore/private/qabstractitemmodel_p.h>

#include <variant>

QT_BEGIN_NAMESPACE

class QRangeModelPrivate : QAbstractItemModelPrivate
{
    Q_DECLARE_PUBLIC(QRangeModel)

public:
    explicit QRangeModelPrivate(std::unique_ptr<QRangeModelImplBase, QRangeModelImplBase::Deleter> impl)
        : impl(std::move(impl))
    {
        this->impl->call<QRangeModelImplBase::InterfaceVersion>(m_interfaceVersion);
        if (m_interfaceVersion >= QT_VERSION_CHECK(6, 12, 0)) {
            m_supportedDropActions = this->impl->call<QRangeModelImplBase::AdjustSupportedDropActions>(
                m_supportedDropActions
            );
        }
    }

    std::unique_ptr<QRangeModelImplBase, QRangeModelImplBase::Deleter> impl;
    friend class QRangeModelImplBase;

    static QRangeModelPrivate *get(QRangeModel *model) { return model->d_func(); }
    static const QRangeModelPrivate *get(const QRangeModel *model) { return model->d_func(); }

    mutable QHash<int, QByteArray> m_roleNames;
    QRangeModel::AutoConnectPolicy m_autoConnectPolicy = QRangeModel::AutoConnectPolicy::None;
    bool m_dataChangedDispatchBlocked = false;
    int m_interfaceVersion = -1;
    int m_sortRole = Qt::DisplayRole;
    std::optional<QCollator> m_sortCollator;
    mutable std::optional<QStringList> m_mimeTypes;
    Qt::DropActions m_supportedDragActions = Qt::CopyAction;
    Qt::DropActions m_supportedDropActions = Qt::CopyAction;

    static void emitDataChanged(const QModelIndex &index, int role)
    {
        const auto *model = static_cast<const QRangeModel *>(index.model());
        if (!get(model)->m_dataChangedDispatchBlocked)
            const_cast<QRangeModel *>(model)->dataChanged(index, index, {role});
    }

    static bool compareModelIndex(const QModelIndex &left, const QModelIndex &right);
};

struct PropertyChangedHandler
{
    PropertyChangedHandler(const QPersistentModelIndex &index, int role)
        : storage{Data{index, role}}
    {}

    // move-only
    ~PropertyChangedHandler() = default;
    PropertyChangedHandler(PropertyChangedHandler &&other) noexcept
        : connection(std::move(other.connection)), storage(std::move(other.storage))
    {
        Q_ASSERT(std::holds_alternative<Data>(storage));
        // A moved-from handler is essentially a reference to the moved-to
        // handler (which lives inside QSlotObject/QCallableObject). This
        // way we can update the stored handler with the created connection.
        other.storage = this;
    }
    PropertyChangedHandler &operator=(PropertyChangedHandler &&) = delete;
    PropertyChangedHandler(const PropertyChangedHandler &) = delete;
    PropertyChangedHandler &operator=(const PropertyChangedHandler &) = delete;

    // we can assign a connection to a moved-from handler to update the
    // handler stored in the QSlotObject/QCallableObject.
    PropertyChangedHandler &operator=(QMetaObject::Connection &&connection)
    {
        Q_ASSERT(std::holds_alternative<PropertyChangedHandler *>(storage));
        std::get<PropertyChangedHandler *>(storage)->connection = std::move(connection);
        return *this;
    }

    void operator()();

private:
    QMetaObject::Connection connection;
    struct Data
    {
        QPersistentModelIndex index;
        int role = -1;
    };
    std::variant<PropertyChangedHandler *, Data> storage;
};

void PropertyChangedHandler::operator()()
{
    Q_ASSERT(std::holds_alternative<Data>(storage));
    const auto &data = std::get<Data>(storage);
    if (!data.index.isValid()) {
        if (!QObject::disconnect(connection))
            qWarning() << "Failed to break connection for" << Qt::ItemDataRole(data.role);
    } else {
        QRangeModelPrivate::emitDataChanged(data.index, data.role);
    }
}

struct ConstPropertyChangedHandler
{
    ConstPropertyChangedHandler(const QModelIndex &index, int role)
        : index(index), role(role)
    {}

    // move-only
    ~ConstPropertyChangedHandler() = default;
    ConstPropertyChangedHandler(ConstPropertyChangedHandler &&other) noexcept = default;

    void operator()() { QRangeModelPrivate::emitDataChanged(index, role); }

private:
    QModelIndex index;
    int role = -1;
};

QRangeModel::QRangeModel(QRangeModelImplBase *impl, QObject *parent)
    : QAbstractItemModel(*new QRangeModelPrivate({impl, {}}), parent)
{
}

QRangeModelImplBase *QRangeModelImplBase::getImplementation(QRangeModel *model)
{
    return model->d_func()->impl.get();
}

const QRangeModelImplBase *QRangeModelImplBase::getImplementation(const QRangeModel *model)
{
    return model->d_func()->impl.get();
}

QScopedValueRollback<bool> QRangeModelImplBase::blockDataChangedDispatch()
{
    return QScopedValueRollback(m_rangeModel->d_func()->m_dataChangedDispatchBlocked, true);
}

int QRangeModelImplBase::sortRole() const
{
    return m_rangeModel->sortRole();
}

const QCollator *QRangeModelImplBase::sortCollator() const
{
    const QRangeModelPrivate *d = QRangeModelPrivate::get(m_rangeModel);
    return d->m_sortCollator ? &d->m_sortCollator.value() : nullptr;
}

QVariant QRangeModelImplBase::convertMatchValue(const QVariant &value, Qt::MatchFlags flags)
{
    QVariant matchValue = value;
    const Qt::CaseSensitivity cs = flags & Qt::MatchCaseSensitive
                                 ? Qt::CaseSensitive : Qt::CaseInsensitive;
    switch ((flags & Qt::MatchTypeMask).toInt()) {
#if QT_CONFIG(regularexpression)
    case Qt::MatchRegularExpression:
    case Qt::MatchWildcard:
        if (value.metaType() != QMetaType::fromType<QRegularExpression>()) {
            QRegularExpression rx;
            if (flags & Qt::MatchWildcard) {
                rx.setPattern(QRegularExpression::wildcardToRegularExpression(
                        value.toString(), QRegularExpression::NonPathWildcardConversion));
            } else {
                rx.setPattern(value.toString());
            }
            if (cs == Qt::CaseInsensitive)
                rx.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
            matchValue = rx;
        }
        break;
#endif // QT_CONFIG(regularexpression)
    case Qt::MatchStartsWith:
    case Qt::MatchEndsWith:
    case Qt::MatchFixedString:
    case Qt::MatchContains:
        matchValue.convert(QMetaType::fromType<QString>());
        break;
    default:
        break;
    }
    return matchValue;
}

bool QRangeModelImplBase::matchValue(const QString &itemData, const QVariant &value,
                                     Qt::MatchFlags flags)
{
    // QString or regular expression based matching
    const uint matchType = (flags & Qt::MatchTypeMask).toInt();
    const Qt::CaseSensitivity cs = flags & Qt::MatchCaseSensitive
                                 ? Qt::CaseSensitive : Qt::CaseInsensitive;
    switch (matchType) {
#if QT_CONFIG(regularexpression)
    case Qt::MatchRegularExpression:
    case Qt::MatchWildcard:
        return itemData.contains(value.toRegularExpression());
#endif // QT_CONFIG(regularexpression)
    case Qt::MatchStartsWith:
        return itemData.startsWith(value.toString(), cs);
    case Qt::MatchEndsWith:
        return itemData.endsWith(value.toString(), cs);
    case Qt::MatchFixedString:
        return itemData.compare(value.toString(), cs) == 0;
    case Qt::MatchContains:
        return itemData.contains(value.toString(), cs);
    default:
        return false;
    }
}

/*!
    \internal

    Using \a metaObject, return a mapping of roles to the matching QMetaProperties.
*/
QHash<int, QMetaProperty> QRangeModelImplBase::roleProperties(const QAbstractItemModel &model,
                                                              const QMetaObject &metaObject)
{
    const auto roles = model.roleNames();
    QHash<int, QMetaProperty> result;
    for (auto &&[role, roleName] : roles.asKeyValueRange()) {
        if (role == Qt::RangeModelDataRole)
            continue;
        result[role] = metaObject.property(metaObject.indexOfProperty(roleName));
    }
    return result;
}

QHash<int, QMetaProperty> QRangeModelImplBase::columnProperties(const QMetaObject &metaObject)
{
    QHash<int, QMetaProperty> result;
    const int propertyOffset = metaObject.propertyOffset();
    for (int p = propertyOffset; p < metaObject.propertyCount(); ++p)
        result[p - propertyOffset] = metaObject.property(p);
    return result;
}

QRangeModelDetails::AutoConnectContext::~AutoConnectContext() = default;

template <auto Handler>
static bool connectPropertiesHelper(const QModelIndex &index, const QObject *item,
                                    QRangeModelDetails::AutoConnectContext *context,
                                    const QHash<int, QMetaProperty> &properties)
{
    if (!item)
        return true;

    auto connect = [item, context](const QModelIndex &cell, int role, const QMetaProperty &property) {
        if (property.hasNotifySignal()) {
            if (!Handler(cell, item, context, role, property))
                return false;
        } else {
            qWarning() << "Property" << property.name() << "for" << Qt::ItemDataRole(role)
                       << "at" << cell << "has no notify signal";
        }
        return true;
    };

    if (context->mapping == QRangeModelDetails::AutoConnectContext::AutoConnectMapping::Roles) {
        for (auto &&[role, property] : properties.asKeyValueRange())
            connect(index, role, property);
    } else {
        for (auto &&[column, property] : properties.asKeyValueRange())
            connect(index.siblingAtColumn(column), Qt::DisplayRole, property);
    }
    return true;
}

bool QRangeModelImplBase::connectProperty(const QModelIndex &index, const QObject *item,
                                          QRangeModelDetails::AutoConnectContext *context,
                                          int role, const QMetaProperty &property)
{
    if (!item)
        return true; // nothing to do, continue
    PropertyChangedHandler handler{index, role};
    auto connection = property.enclosingMetaObject()->connect(item, property.notifySignal(),
                                                              context, std::move(handler));
    if (!connection) {
        qWarning() << "Failed to connect to" << item << property.name();
        return false;
    } else {
        // handler is now in moved-from state, and acts like a reference to
        // the handler that is stored in the QSlotObject/QCallableObject.
        // This assignment updates the stored handler's connection with the
        // QMetaObject::Connection handle, and should look harmless for
        // static analyzers.
        handler = std::move(connection);
    }
    return true;
}

bool QRangeModelImplBase::connectProperties(const QModelIndex &index, const QObject *item,
                                            QRangeModelDetails::AutoConnectContext *context,
                                            const QHash<int, QMetaProperty> &properties)
{
    return connectPropertiesHelper<QRangeModelImplBase::connectProperty>(index, item, context, properties);
}

bool QRangeModelImplBase::connectPropertyConst(const QModelIndex &index, const QObject *item,
                                               QRangeModelDetails::AutoConnectContext *context,
                                               int role, const QMetaProperty &property)
{
    if (!item)
        return true; // nothing to do, continue
    ConstPropertyChangedHandler handler{index, role};
    if (!property.enclosingMetaObject()->connect(item, property.notifySignal(),
                                                 context, std::move(handler))) {
        qWarning() << "Failed to connect to" << item << property.name();
        return false;
    } else {
        return true;
    }
}

bool QRangeModelImplBase::connectPropertiesConst(const QModelIndex &index, const QObject *item,
                                                 QRangeModelDetails::AutoConnectContext *context,
                                                 const QHash<int, QMetaProperty> &properties)
{
    return connectPropertiesHelper<QRangeModelImplBase::connectPropertyConst>(index, item, context, properties);
}

namespace QRangeModelDetails
{
Q_CORE_EXPORT QVariant qVariantAtIndex(const QModelIndex &index)
{
    QModelRoleData result[] = {
        QModelRoleData{Qt::RangeModelAdapterRole},
        QModelRoleData{Qt::RangeModelDataRole},
        QModelRoleData{Qt::DisplayRole},
    };
    index.multiData(result);
    QVariant variant;
    size_t r = 0;
    do {
        variant = result[r].data();
        ++r;
    } while (!variant.isValid() && r < std::size(result));

    return variant;
}
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

    The range must be provided when constructing the model and can be provided
    by value, reference wrapper, or pointer. How the model was constructed
    defines whether changes through the model API will modify the original
    data. Use QRangeModelAdapter to implicitly construct a model while also
    having direct, type-safe, and convenient access to the model as a range.

    When constructed by value, the model makes a copy of the range, and
    QAbstractItemModel APIs that modify the model, such as setData() or
    insertRows(), have no impact on the original range.

    \snippet qrangemodel/main.cpp value

    Changes made to the data can be monitored by connecting to the signals
    emitted by the model, such as \l{QAbstractItemModel}{dataChanged()}.

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
    maintains. Use QRangeModelAdapter to safely interact with the underlying
    range while keeping the model updated.

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

    \snippet qrangemodel/specialize.cpp color_gadget_decl
    \snippet qrangemodel/specialize.cpp color_gadget_impl
    \snippet qrangemodel/specialize.cpp color_gadget_end

    When used in a table, this is the default representation for gadgets:

    \snippet qrangemodel/specialize.cpp color_gadget_table

    When used in a list, these types are however by default represented as
    multi-column rows, with each property represented as a separate column. To
    force a gadget to be represented as a multi-role item in a list, declare
    the gadget as a multi-role type by specializing QRoleModel::RowOptions,
    with a \c{static constexpr auto rowCategory} member variable set to
    MultiRoleItem.

    \snippet qrangemodel/specialize.cpp color_gadget_decl
    \dots
    \snippet qrangemodel/specialize.cpp color_gadget_end
    \snippet qrangemodel/specialize.cpp color_gadget_row_options_decl
    \snippet qrangemodel/specialize.cpp color_gadget_row_options_rowCategory
    \snippet qrangemodel/specialize.cpp color_gadget_row_options_end_decl

    You can also wrap such types into a single-element tuple, turning the list
    into a table with a single column:

    \snippet qrangemodel/specialize.cpp color_gadget_single_column

    In this case, note that direct access to the elements in the list data
    needs to use \c{std::get}:

    \snippet qrangemodel/specialize.cpp color_gadget_single_column_access_get

    or alternatively a structured binding:

    \snippet qrangemodel/specialize.cpp color_gadget_single_column_access_sb

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
    or QObject type, or a type that implements \l{the C++ tuple protocol}.

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
    to a row item; and the \c{childRows()} function has to return a reference
    to a const \c{std::optional} that can hold the optional child range.

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

    \section1 Customization by template specialization

    QRangeModel declares two nested templates types that you can specialize to
    override default behavior.

    The RowOptions template we have already introduced above is for customizing
    functionality that is specific to the row-type, such as header data, or
    that should apply uniformly to all items in a row, such as default flags.
    The ItemAccess template allows customizing item type specific behavior,
    such as reading or writing role data.

    These two templates always need to be specialized for the underlying type,
    so even if the range holds rows of type \c{Row} as
    \c{std::unique_ptr<Row>}, or items of type \c{Item} as
    \c{std::shared_ptr<Item>}, the specialization needs to be for \c{Row} and
    \c{Item}.

    \snippet qrangemodel/specialize.cpp color_gadget_item_access_decl
    \dots
    \snippet qrangemodel/specialize.cpp color_gadget_item_access_end_decl

    \snippet qrangemodel/specialize.cpp color_gadget_item_access_use

    \note In C++ you should only ever specialize templates for types that you
    own, and not for standard or Qt types. Create subclasses or aggregates for
    types you don't control if you need to customize behavior for those types.
    Note that a type alias is not a distinct type, and you should not
    specialize templates for alias types. However, it is allowed to create a
    specialization of a standard or Qt container with a type that you own, such
    as a \c{std::vector<MyGadget>}.

    \section2 Row and item specific flags

    The default implementation of flags() returns a combination of
    Qt::ItemIsSelectable, Qt::ItemIsEnabled, and - except for items at column 0
    of a tree model - Qt::ItemNeverHasChildren. The Qt::ItemIsEditable and
    Qt::ItemIsDropEnabled flags are set for all items in models that are not
    read-only, and Qt::ItemIsDragEnabled is set as long as the model supports
    at least one \l{mimeTypes()}{mime type}.

    To customize the default behavior for all items in a row, specialize the
    RowOptions template. To access per-item flag data, specialize the
    ItemAccess template. You can do both: RowOptions can set flags that are
    common for all items in a row, and ItemAccess can set flags for items
    backed by a specific type.

    \section2 Drag'n'drop handling

    Since Qt 6.12, QRangeModel implements the flags() virtual function to set
    the Qt::ItemIsDragEnabled flag for all valid items in a model, as long as
    the model supports at least one \l{mimeTypes()}{mime type}. This is the
    default, as QAbstractItemModel provides the Qt-internal
    "application/x-qabstractitemmodeldatalist" mime type. In addition, the
    Qt::ItemIsDropEnabled flag is set for items in such a model as long as that
    model is not read-only. This includes the non-existent item at the invalid
    index, so users can drop data into empty areas of a view to append the
    data.

    Implementing support for additional mime types can be done without
    subclassing QRangeModel and overriding the respective virtual functions.
    QRangeModel will detect and use functions in the ItemAccess and RowOptions
    customization templates to encode rows or items as \l{ItemAccess::mimeData()}
    {mime data}, and to \l{ItemAccess::dropMimeData()}{decode mime data} into
    sequences of rows or items. These customization functions can operate
    directly on the row and item types, without taking detours through QModelIndex
    and QVariant.

    This way, the code for serializing data can be type-safe, and can stay with
    the implementation of the type that is used to store the data.

    In addition, \l{supportedDragActions} and \l{supportedDropActions} are
    properties that can be configured for each QRangeModel instance.

    \section1 Advanced C++ topics

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

    \section2 Binary compatibility considerations

    QRangeModel is not a template class. Passing QRangeModel instances (by
    pointer or reference, as with all QObject classes) through library APIs, or
    storing QRangeModel by value in a public class of a library, is safe.

    However, the QRangeModel constructor is a template and inline, and the
    internal implementation that is specialized on the type of the range the
    model operates on is instantiated in the constructor. You should not call
    the constructor in an inline-implementation of a library API. It results in
    ODR violations, and might break binary compatibility of that library if the
    Qt version it gets built against is different from the Qt version an
    application using that library is built against.

    New and optimized implementations of virtual functions introduced in later
    version of Qt might also not be used if QRangeModel detects that the
    implementation was compiled against an older version of Qt. For instance,
    the implementations of sort() and match() are new in Qt 6.12, but will not
    be called by an application that was compiled against Qt 6.11, even if the
    Qt library used is Qt 6.12. To benefit from such new overrides, recompile
    your application.

    \sa {Model/View Programming}
*/

/*!
    \class QRangeModel::RowOptions
    \inmodule QtCore
    \ingroup model-view
    \brief The RowOptions template provides a customization point to control
           how QRangeModel represents types used as rows.
    \since 6.10

    RowOptions\<T\> is a struct template where \a T specifies the row type.
    Specialize this template for the type used in your range, and add the
    relevant members.

    \snippet qrangemodel/specialize.cpp color_gadget_row_options_decl
    \dots
    \snippet qrangemodel/specialize.cpp color_gadget_row_options_end_decl

    \sa ItemAccess
*/

/*!
    \variable QRangeModel::RowOptions::rowCategory

    Set this static compile-time constant to one of the values in the
    RowCategory enumerator to define how QRangeModel should interpret the
    elements of the range. Not providing this constant is the equivalent of
    RowCategory::Default.

    \snippet qrangemodel/specialize.cpp color_gadget_decl
    \dots
    \snippet qrangemodel/specialize.cpp color_gadget_end

    \snippet qrangemodel/specialize.cpp color_gadget_row_options_decl
    \snippet qrangemodel/specialize.cpp color_gadget_row_options_rowCategory
    \snippet qrangemodel/specialize.cpp color_gadget_row_options_end_decl

    If the \c{rowCategory} is set to \l{QRangeModel::RowCategory}{MultiRoleItem},
    then none of the other members will have any effect.

    \sa ItemAccess
*/

/*!
    \fn template <typename T> QVariant QRangeModel::RowOptions<T>::headerData(int section, int role)
    \since 6.12

    Implement this class member to return the header data QRangeModel should
    return for the \a role of the \a section in the horizontal header.

    \snippet qrangemodel/specialize.cpp color_gadget_row_options_decl
    \snippet qrangemodel/specialize.cpp color_gadget_row_options_headerData
    \snippet qrangemodel/specialize.cpp color_gadget_row_options_end_decl

    If this member is not provided, then QRangeModel returns type-specific default
    values from the headerData() implementation.

    \sa QAbstractItemModel::headerData()
*/

/*!
    \fn template <typename T> Qt::ItemFlags QRangeModel::RowOptions<T>::flags(const T &row)
    \since 6.12

    Implement this class member to return the \l{QAbstractItemModel::flags}{flags}
    for all items in \a row.

    \snippet qrangemodel/specialize.cpp color_gadget_row_options_decl
    \snippet qrangemodel/specialize.cpp color_gadget_row_options_flags
    \snippet qrangemodel/specialize.cpp color_gadget_row_options_end_decl

    This will be overwritten by a customization of \l{ItemAccess}{ItemAccess::flags}.

    If this member is not provided, then QRangeModel computes the flags based
    on the range it was constructed from.

    \sa QAbstractItemModel::flags()
*/

/*!
    \fn template <typename T> static QStringList QRangeModel::RowOptions<T>::mimeTypes()
    \since 6.12

    Implement this class member to return the list of \l{QAbstractItemModel::mimeTypes}
    {mime types} that a model holding rows of type \a T can use to represent the
    model data during drag'n'drop operations.

    \snippet qrangemodel/specialize.cpp color_gadget_row_options_decl
    \snippet qrangemodel/specialize.cpp color_gadget_row_options_mimeTypes
    \snippet qrangemodel/specialize.cpp color_gadget_row_options_end_decl

    If this member is not provided, then QRangeModel returns the default mime
    type, "application/x-qabstractitemmodeldatalist", as supported by
    QAbstractItemModel. If the returned list does not include that mime type,
    then it will not be supported.

    \sa QAbstractItemModel::mimeTypes()
*/

/*!
    \fn template <typename T> QMimeData *QRangeModel::RowOptions<T>::mimeData(const auto &range)
    \fn template <typename T> QMimeData *QRangeModel::RowOptions<T>::mimeData(const QModelIndex &range)
    \since 6.12

    Implement one of these class members to return the \l{QAbstractItemModel::mimeData()}
    {mime data} for the rows in the provided \a range.

    If the generic version is provided, then the iterator over \a range is
    bidirectional, and dereferences to a pair of a row of type \a T and the
    corresponding QModelIndex.

    \snippet qrangemodel/specialize.cpp color_gadget_row_options_decl
    \snippet qrangemodel/specialize.cpp color_gadget_row_options_mimeData
    \snippet qrangemodel/specialize.cpp color_gadget_row_options_end_decl
    \code
    for (const auto &[row, index] : range) {
        // ...
    }
    \endcode

    Fully selected rows will be included in the range only once, paired with an
    \c index that holds the row number and parent, and a column of -1.
    Partially selected rows are included in the range multiple times, once for
    each selected column.

    The entries in \a range are sorted in logical order, from top-most to last
    row, and from left-most column to last column.

    \sa QAbstractItemModel::mimeData()
*/

/*!
    \fn template <typename T> bool QRangeModel::RowOptions<T>::canDropMimeData(const QMimeData *data)
    \fn template <typename T> bool QRangeModel::RowOptions<T>::canDropMimeData(const QMimeData *data, Qt::DragAction action, int row, int column, const QModelIndex &parent)
    \since 6.12

//! [specialize-canDropMimeData]
    Implement one of these class members to return whether \c data can be dropped
    into the model. If the simplified version is implemented, then QRangeModel will
    validate that \a action is one of the \l{QRangeModel::supportedDropActions}
    {supported drop actions}. In the full version the implementation can return
    different results based on the position of the drop as specified by \a row,
    \a column, and \a parent.

    This member is optional and not required for drag'n'drop customization.
    The default behavior returns whether \c data holds one of the supported mime
    types.
//! [specialize-canDropMimeData]

    \sa QAbstractItemModel::canDropMimeData()
*/

/*!
    \fn template <typename T> auto QRangeModel::RowOptions<T>::dropMimeData(const QMimeData *data, auto inserter)
    \fn template <typename T> auto QRangeModel::RowOptions<T>::dropMimeData(const QMimeData *data, Qt::DragAction action, int row, int column, const QModelIndex &parent, auto inserter)
    \since 6.12

    Implement one of these class members to decode the relevant entries in \a
    data into a sequence of rows of type \a T, and drop these rows into the
    model by assigning each to the provided \a inserter. Return whether the
    operation was successful.

    \snippet qrangemodel/specialize.cpp color_gadget_row_options_decl
    \snippet qrangemodel/specialize.cpp color_gadget_row_options_dropMimeData
    \snippet qrangemodel/specialize.cpp color_gadget_row_options_end_decl

    The \a inserter behaves like a \l{https://cppreference.com/cpp/iterator/back_insert_iterator}
    {std::back_insert_iterator} and can be used with e.g. \c{std::copy}. The
    above snippets omits error handling, which perhaps would require storing the
    decoded items in a separate list that can be discarded in case of an error.
    The following would efficiently move the decoded rows into the model:

    \code
    static bool dropMimeData(const QMimeData *mimeData, auto inserter)
    {
        QList<ColorEntry> decodedEntries;
        // read stream and populate decodedEntries

        if (stream.hasError())
            return false;
        std::copy(std::move_iterator(decodedRows.begin()), std::move_iterator(decodedRows.end()),
                    inserter);
        return true;
    }
    \endcode

    Optionally, decoded rows can be paired with the relative row number it
    should have in the inserted range of rows.

    \code
    int row = 0;
    for (const auto &decodedRow : decodedRows) {
        inserter = {decodedRow, row};
        row += 2;
    }
    \endcode

    This implementation keeps an empty row between each decoded row.

//! [specialize-dropMimeData-return]
    Return one of the \l{QRangeModel::}{DropOperation} values to provide a hint
    to QRangeModel for how the dropped rows should be applied to the model.
    Alternatively, simply return \c{true} (the equivalent of
    \l{QRangeModel::}{DropOperation::Automatic}) or \c{false} (the equivalent of
    \l{QRangeModel::}{DropOperation::DontDrop}).
//! [specialize-dropMimeData-return]

    \sa QAbstractItemModel::dropMimeData()
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
    \class QRangeModel::ItemAccess
    \inmodule QtCore
    \ingroup model-view
    \brief The ItemAccess template provides a customization point to control
           how QRangeModel accesses role data of individual items.
    \since 6.11

    ItemAccess\<T\> is a struct template where \a T specifies the item type.
    Specialize this template for the type used in your data structure, and
    implement the relevant class member functions.

    \snippet qrangemodel/specialize.cpp color_gadget_item_access_decl
    \snippet qrangemodel/specialize.cpp color_gadget_item_access_flags
    \snippet qrangemodel/specialize.cpp color_gadget_item_access_readRole
    \snippet qrangemodel/specialize.cpp color_gadget_item_access_writeRole
    \dots
    \snippet qrangemodel/specialize.cpp color_gadget_item_access_end_decl

    A specialization of this type will take precedence over any predefined
    behavior, and over a corresponding specialization of \l{QRangeModel::}{RowOptions}.

    \note Do not specialize this template for types you do not own.
*/

/*!
    \fn template <typename T> QVariant QRangeModel::ItemAccess<T>::readRole(const T &item, int role)

    Implement this class member to return the data in \a item for the requested
    \a role.

    \snippet qrangemodel/specialize.cpp color_gadget_item_access_decl
    \snippet qrangemodel/specialize.cpp color_gadget_item_access_readRole
    \snippet qrangemodel/specialize.cpp color_gadget_item_access_end_decl

    Types for which ItemAccess is specialized with a \c{readRole} implementation
    are implicitly interpreted as \l{RowCategory}{multi-role items}.

    \sa QAbstractItemModel::data()
*/

/*!
    \fn template <typename T> bool QRangeModel::ItemAccess<T>::writeRole(T &item, const QVariant &value, int role)

    Implement this class member to set the data of \a item for the requested
    \a role to the provided \a value, and return whether the change was
    successful.

    \snippet qrangemodel/specialize.cpp color_gadget_item_access_decl
    \snippet qrangemodel/specialize.cpp color_gadget_item_access_writeRole
    \snippet qrangemodel/specialize.cpp color_gadget_item_access_end_decl

    \sa QAbstractItemModel::setData()
*/

/*!
    \fn template <typename T> Qt::ItemFlags QRangeModel::ItemAccess<T>::flags(const T &item)
    \since 6.12

    Implement this class member to return the \l{QAbstractItemModel::flags}{flags}
    for \a item.

    \snippet qrangemodel/specialize.cpp color_gadget_item_access_decl
    \snippet qrangemodel/specialize.cpp color_gadget_item_access_flags
    \snippet qrangemodel/specialize.cpp color_gadget_item_access_end_decl

    \sa QAbstractItemModel::flags()
*/

/*!
    \fn template <typename T> static QStringList QRangeModel::ItemAccess<T>::mimeTypes()
    \since 6.12

    Implement this class member to return the list of \l{QAbstractItemModel::mimeTypes}
    {mime types} that a model holding items of type \a T can use to represent the
    model data during drag'n'drop operations.

    \snippet qrangemodel/specialize.cpp color_gadget_item_access_decl
    \snippet qrangemodel/specialize.cpp color_gadget_item_access_mimeTypes
    \snippet qrangemodel/specialize.cpp color_gadget_item_access_end_decl

    If the list includes the \l{QAbstractItemModel::mimeTypes()}{default mime type}
    that Qt uses for drag'n'drop data, then QRangeModel will take care of the
    encoding and decoding of data for that mime type automatically.

//! [specialize-ItemAccess-dragdrop]
    \note This specialization will only be considered when all items in the
    model are backed by items of type \a T. For heterogenous models where
    different columns provide different data types, specialize
    \l{QRangeModel::}{RowOptions} instead.
//! [specialize-ItemAccess-dragdrop]

    \sa QAbstractItemModel::mimeTypes()
*/

/*!
    \fn template <typename T> QMimeData *QRangeModel::ItemAccess<T>::mimeData(const auto &range)
    \fn template <typename T> QMimeData *QRangeModel::ItemAccess<T>::mimeData(const QModelIndex &range)
    \since 6.12

    Implement one of these class members to return the \l{QAbstractItemModel::mimeData()}
    {mime data} for the items in the provided \a range.

    If the generic version is provided, then the iterator over \a range is
    bidirectional, and dereferences to a pair with an item of type \a T and the
    corresponding QModelIndex.

    \snippet qrangemodel/specialize.cpp color_gadget_item_access_decl
    \snippet qrangemodel/specialize.cpp color_gadget_item_access_mimeData
    \snippet qrangemodel/specialize.cpp color_gadget_item_access_end_decl

    The entries in \a range are sorted in logical order, from top-most to last row,
    and from left-most column to last column.

    \include qrangemodel.cpp specialize-ItemAccess-dragdrop

    \sa QAbstractItemModel::mimeData()
*/

/*!
    \fn template <typename T> bool QRangeModel::ItemAccess<T>::canDropMimeData(const QMimeData *data)
    \fn template <typename T> bool QRangeModel::ItemAccess<T>::canDropMimeData(const QMimeData *data, Qt::DragAction action, int row, int column, const QModelIndex &parent)
    \since 6.12

    \include qrangemodel.cpp specialize-canDropMimeData

    \include qrangemodel.cpp specialize-ItemAccess-dragdrop

    \sa QAbstractItemModel::canDropMimeData()
*/

/*!
    \fn template <typename T> auto QRangeModel::ItemAccess<T>::dropMimeData(const QMimeData *data, auto inserter)
    \fn template <typename T> auto QRangeModel::ItemAccess<T>::dropMimeData(const QMimeData *data, Qt::DragAction action, int row, int column, const QModelIndex &parent, auto inserter)
    \since 6.12

    Implement one of these class members to decode the relevant entries in \a data
    into a sequence of items of type \a T, and drop these rows into the
    model by assigning each to the provided \a inserter. Return whether the
    operation was successful.

    \snippet qrangemodel/specialize.cpp color_gadget_item_access_decl
    \snippet qrangemodel/specialize.cpp color_gadget_item_access_dropMimeData
    \snippet qrangemodel/specialize.cpp color_gadget_item_access_end_decl

    Optionally, decoded items can be paired with a row and column value relative
    to the drop location. If the mime data includes positional information, then
    this makes it possible to maintain the "shape" of the items.

    \code
    for (const auto &decodedItem: decodedItems) {
        inserter = {
            decodedItem,
            {
                relativeRow,
                relativeColumn
            }
        };
    }
    \endcode

    Implement the version with \a action, \a row, \a column, and \a parent
    parameters for full control over the operation.

    \include qrangemodel.cpp specialize-dropMimeData-return

    \include qrangemodel.cpp specialize-ItemAccess-dragdrop

    \sa QAbstractItemModel::dropMimeData()
*/

/*!
    \fn template <typename Range, QRangeModelDetails::if_table_range<Range>> QRangeModel::QRangeModel(Range &&range, QObject *parent)
    \fn template <typename Range, QRangeModelDetails::if_tree_range<Range>> QRangeModel::QRangeModel(Range &&range, QObject *parent)
    \fn template <typename Range, typename Protocol, QRangeModelDetails::if_tree_range<Range, Protocol>> QRangeModel::QRangeModel(Range &&range, Protocol &&protocol, QObject *parent)

    Constructs a QRangeModel instance that operates on the data in \a range.
    The \a range has to be a sequential range for which the compiler finds
    \c{begin} and \c{end} overloads through
    \l{https://en.cppreference.com/w/cpp/language/adl.html}{argument dependent
    lookup}, or for which \c{std::begin} and \c{std::end} are implemented. If
    \a protocol is provided, then the model will represent the range as a tree
    using the protocol implementation. The model instance becomes a child of \a
    parent.

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
    and and passed on to a view. Such modifications will not emit signals
    necessary to keep model users (other models or views) synchronized with the
    model, resulting in inconsistent results, undefined behavior, and crashes.
    Use QRangeModelAdapter to safely interact with the underlying range while
    keeping the model updated.

    \sa QRangeModelAdapter
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

    Models that return a non-empty list of \l{mimeTypes()}{mimeTypes()} also
    set the Qt::ItemIsDragEnabled, and - unless read-only - the
    Qt::ItemIsDropEnabled flag.

    Flat models set the Qt::ItemNeverHasChildren for all items, while
    hierarchical models set that flag for all items in columns above 0.

    To customize the flags for your own data types, provide a specialization
    of RowOptions and/or ItemAccess for your row or item types.

    \sa Qt::ItemFlags, RowOptions, ItemAccess
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
    \enum QRangeModel::DropOperation
    \since 6.12

    This enum defines how data decoded in a dropMimeData() customization gets
    written to the model.

    \value DontDrop             The data should not be added to the model.
    \value Automatic            Qt determines how data gets added to the model.
    \value OverwriteAndIgnore   Overwrite the dropped-on item and following items,
                                and discard any dropped data that doesn't fit.
    \value OverwriteAndExtend   Overwrite the dropped-on item and following items,
                                and grow the model to fit all dropped data.
    \value InsertAsSiblings     Insert all dropped data as siblings of the dropped-on
                                item.
    \value InsertAsChildren     Insert all dropped data as children of the dropped-on
                                item.

    A dropMimeData() customization can return a bool value instead, in which
    case \c{false} maps to the DontDrop value, and \c{true} to Automatic.

    \sa canDropMimeData(), dropMimeData()
*/

/*!
    \reimp

    \sa RowOptions::canDropMimeData() ItemAccess::canDropMimeData()
*/
bool QRangeModel::canDropMimeData(const QMimeData *data, Qt::DropAction action,
                                  int row, int column, const QModelIndex &parent) const
{
    Q_D(const QRangeModel);
    if (d->m_interfaceVersion < QT_VERSION_CHECK(6, 12, 0))
        return QAbstractItemModel::canDropMimeData(data, action, row, column, parent);
    return d->impl->call<QRangeModelImplBase::CanDropMimeData>(data, action, row, column, parent);
}

/*!
    \reimp

    \sa RowOptions::dropMimeData() ItemAccess::dropMimeData()
*/
bool QRangeModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
                               int row, int column, const QModelIndex &parent)
{
    if (!data)
        return false;
    if (action == Qt::IgnoreAction)
        return true;

    Q_D(QRangeModel);
    if (d->m_interfaceVersion < QT_VERSION_CHECK(6, 12, 0))
        return QAbstractItemModel::dropMimeData(data, action, row, column, parent);
    if (d->impl->call<QRangeModelImplBase::DropMimeData>(data, action, row, column, parent))
        return true;

    // failing that, insert the data as new indexes. A row of -1 indicates
    // that data should be appended to the model
    if (row == -1)
        row = rowCount(parent);
#if !defined(QT_NO_DATASTREAM)
    const QString defaultFormat = QAbstractItemModel::mimeTypes().at(0);
    if (data->hasFormat(defaultFormat)) {
        QByteArray encoded = data->data(defaultFormat);
        QDataStream stream(&encoded, QDataStream::ReadOnly);
        return decodeData(row, column, parent, stream);
    }
#endif
    return false;
}

bool QRangeModelImplBase::dropDataOnItem(const QMimeData *data, const QModelIndex &index)
{
#if !defined(QT_NO_DATASTREAM)
    const QString defaultFormat = m_rangeModel->QAbstractItemModel::mimeTypes().at(0);
    if (data->hasFormat(defaultFormat)) {
        QByteArray encoded = data->data(defaultFormat);
        QDataStream stream(&encoded, QDataStream::ReadOnly);
        return m_rangeModel->d_func()->dropOnItem(index, stream);
    }
#endif
    return false;
}

// QModelIndex::operator< is not useful as it compares the internal data pointer.
// We need to compare indexes based on the row path, taking into account
// that only items at column zero can have children.
bool QRangeModelPrivate::compareModelIndex(const QModelIndex &left, const QModelIndex &right)
{
    if (!left.isValid())
        return right.isValid();
    if (!right.isValid())
        return false;
    const QModelIndex leftCol0 = left.column() ? left.siblingAtColumn(0) : left;
    const QModelIndex rightCol0 = right.column() ? right.siblingAtColumn(0) : right;
    const QModelIndex leftParent = leftCol0.parent();
    const QModelIndex rightParent = rightCol0.parent();
    if (leftParent == rightParent) {
        if (left.row() == right.row())
            return left.column() < right.column();
        return left.row() < right.row();
    }
    // parents come before their children
    if (leftCol0 == rightParent)
        return true;
    if (rightCol0 == leftParent)
        return false;

    // indexes are not directly related, so generate and compare their paths
    auto makePath = [](const QModelIndex &index) {
        // we call with col0 indexes and a parent can never be at another column
        Q_ASSERT(index.column() == 0);
        QVarLengthArray<int, 32> path{index.row()};
        QModelIndex parent = index.parent();
        while (parent.isValid()) {
            path.append(parent.row());
            parent = parent.parent();
        }
        std::reverse(path.begin(), path.end());
        return path;
    };
    const auto leftPath = makePath(leftCol0);
    const auto rightPath = makePath(rightCol0);
    return leftPath < rightPath;
}

/*!
    \reimp

    \sa RowOptions::mimeData() ItemAccess::mimeData()
*/
QMimeData *QRangeModel::mimeData(const QModelIndexList &indexes) const
{
    Q_D(const QRangeModel);
    if (indexes.isEmpty())
        return nullptr;

    if (d->m_interfaceVersion < QT_VERSION_CHECK(6, 12, 0))
        return QAbstractItemModel::mimeData(indexes);

    // sort the indexes so that all indexes in the same row are in sequence.
    QModelIndexList groupedList = indexes;
    std::sort(groupedList.begin(), groupedList.end(), QRangeModelPrivate::compareModelIndex);
    QMimeData *data = d->impl->call<QRangeModelImplBase::MimeData>(groupedList);

    // finalize with default mime type
    const QString defaultFormat = QAbstractItemModel::mimeTypes().at(0);
    if (!mimeTypes().contains(defaultFormat))
        return data;

    std::unique_ptr<QMimeData> defaultMimeData(QAbstractItemModel::mimeData(indexes));
    if (!data)
        return defaultMimeData.release();
    // add default mime data into the custom data
    if (defaultMimeData) {
        const QStringList defaultTypes = defaultMimeData->formats();
        for (const auto &defaultType : defaultTypes)
            data->setData(defaultType, defaultMimeData->data(defaultType));
    }
    return data;
}

/*!
    \reimp

    \sa RowOptions::mimeTypes() ItemAccess::mimeTypes()
*/
QStringList QRangeModel::mimeTypes() const
{
    Q_D(const QRangeModel);
    if (!d->m_mimeTypes) {
        d->m_mimeTypes = d->m_interfaceVersion < QT_VERSION_CHECK(6, 12, 0)
                       ? QAbstractItemModel::mimeTypes()
                       : d->impl->call<QRangeModelImplBase::MimeTypes>();
    }
    return *d->m_mimeTypes;
}

/*!
    \reimp

    Returns a list of indexes for the items in the column of \a start where
    the data stored under \a role matches \a value, using the match criteria
    defined by \a flags. Use \a hits = -1 to find all matching items.

    \note This implementation reads data directly from the underlying C++
    range and does not dispatch through overrides of data().
*/
QModelIndexList QRangeModel::match(const QModelIndex &start, int role, const QVariant &value,
                                         int hits, Qt::MatchFlags flags) const
{
    Q_D(const QRangeModel);
    if (d->m_interfaceVersion < QT_VERSION_CHECK(6, 12, 0))
        return QAbstractItemModel::match(start, role, value, hits, flags);
    return d->impl->call<QRangeModelImplBase::Match>(start, role, value, hits, flags);
}

/*!
    \reimp
*/
void QRangeModel::multiData(const QModelIndex &index, QModelRoleDataSpan roleDataSpan) const
{
    Q_D(const QRangeModel);
    if (d->m_interfaceVersion < QT_VERSION_CHECK(6, 11, 0))
        return QAbstractItemModel::multiData(index, roleDataSpan);
    d->impl->call<QRangeModelImplBase::MultiData>(index, roleDataSpan);
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
    if (d->m_autoConnectPolicy != AutoConnectPolicy::None)
        d->impl->call<QRangeModelImplBase::SetAutoConnectPolicy>();

    d->m_roleNames = names;
    endResetModel();
    Q_EMIT roleNamesChanged();
}

void QRangeModel::resetRoleNames()
{
    setRoleNames({});
}

/*!
    \enum QRangeModel::AutoConnectPolicy
    \since 6.11

    This enum defines if and when QRangeModel auto-connects changed-signals for
    properties to the \l{QAbstractItemModel::}{dataChanged()} signal of the
    model. Only properties that match one of the \l{roleNames()}{role names}
    are connected.

    \value None     No connections are made automatically.
    \value Full     The signals for all relevant properties are connected
                    automatically, for all QObject items. This includes QObject
                    items that are added to newly inserted rows and columns.
    \value OnRead   Signals for relevant properties are connected the first time
                    the model reads the property.

    The memory overhead of making automatic connections can be substantial. A
    Full auto-connection does not require any book-keeping in addition to the
    connection itself, but each connection takes memory, and connecting all
    properties of all objects can be very costly, especially if only a few
    properties of a subset of objects will ever change.

    The OnRead connection policy will not connect to objects or properties that
    are never read from (for instance, never rendered in a view), but remembering
    which connections have been made requires some book-keeping overhead, and
    unpredictable memory growth over time. For instance, scrolling down a long
    list of items can easily result in thousands of new connections being made.

    \sa autoConnectPolicy, roleNames()
*/

/*!
    \property QRangeModel::autoConnectPolicy
    \since 6.11
    \brief if and when the model auto-connects to property changed notifications.

    If QRangeModel operates on a data structure that holds the same type of
    QObject subclass as its row or item type, then it can automatically connect
    the properties of the QObjects to the dataChanged() signal. For QObject
    rows, this is done for each column, mapping to the Qt::DisplayRole
    property. For items, this is done for those properties that match one of
    the \l{roleNames()}{role names}.

    By default, the value of this property is \l{QRangeModel::AutoConnectPolicy::}
    {None}, so no such connections are made. Changing the value of this property
    always breaks all existing connections.

    \note Connections are not broken or created if QObjects in the data
    structure that QRangeModel operates on are swapped out.

    \sa roleNames()
*/

QRangeModel::AutoConnectPolicy QRangeModel::autoConnectPolicy() const
{
    Q_D(const QRangeModel);
    return d->m_autoConnectPolicy;
}

void QRangeModel::setAutoConnectPolicy(QRangeModel::AutoConnectPolicy policy)
{
    Q_D(QRangeModel);
    if (d->m_autoConnectPolicy == policy)
        return;

    d->m_autoConnectPolicy = policy;
    d->impl->call<QRangeModelImplBase::SetAutoConnectPolicy>();
    Q_EMIT autoConnectPolicyChanged(policy);
}

/*!
    \reimp

    Sorts the the underlying range in the given \a order, based on the data for
    the \l{sortRole} (Qt::DisplayRole by default) of the items in \a column.

    \note This implementation uses a member function \c{sort(Compare comp)} of
    the C++ range if available (such as in \c{std::list}), or otherwise
    \c{std::stable_sort()} if the range provides random-access iterators. If
    neither is available then the implementation does nothing and returns
    immediately.

    \note Accessing the item does not dispatch the reading of data through
    overrides of data().

    \sa sortRole, QSortFilterProxyModel
*/
void QRangeModel::sort(int column, Qt::SortOrder order)
{
    Q_D(QRangeModel);
    if (d->m_interfaceVersion < QT_VERSION_CHECK(6, 12, 0))
        return QAbstractItemModel::sort(column, order);
    QT_TRY {
        d->impl->call<QRangeModelImplBase::Sort>(column, order);
    } QT_CATCH(const std::bad_alloc &) {
        qCritical("QRangeModel::sort ran out of memory, sort likely incomplete.");
    }
}

/*!
    \property QRangeModel::sortRole
    \since 6.12
    \brief the data role used when sorting items.

    The default value is Qt::DisplayRole.

    \sa sort(), sortCollator, QSortFilterProxyModel
*/
int QRangeModel::sortRole() const
{
    Q_D(const QRangeModel);
    return d->m_sortRole;
}

void QRangeModel::setSortRole(int role)
{
    Q_D(QRangeModel);
    if (d->m_sortRole == role)
        return;
    d->m_sortRole = role;
    Q_EMIT sortRoleChanged(d->m_sortRole);
}

void QRangeModel::resetSortRole()
{
    setSortRole(Qt::DisplayRole);
}

/*!
    \property QRangeModel::sortCollator
    \since 6.12
    \brief the collator that will be used when sorting the model

    The default value of this property is a QCollator for the C-locale.
    Sorting will not be locale aware, and case sensitive. Setting a collator
    will make the sorting locale-aware.

    \sa sort(), sortRole, QSortFilterProxyModel
*/
QCollator QRangeModel::sortCollator() const
{
    Q_D(const QRangeModel);
    return d->m_sortCollator.value_or(QCollator(QLocale::C));
}

void QRangeModel::setSortCollator(const QCollator &collator)
{
    Q_D(QRangeModel);
    if (sortCollator() == collator)
        return;
    d->m_sortCollator = collator;
    Q_EMIT sortCollatorChanged(*d->m_sortCollator);
}

void QRangeModel::resetSortCollator()
{
    Q_D(QRangeModel);
    if (!d->m_sortCollator)
        return;
    d->m_sortCollator = std::nullopt;
    Q_EMIT sortCollatorChanged(sortCollator());
}

/*!
    \reimp
*/
QSize QRangeModel::span(const QModelIndex &index) const
{
    return QAbstractItemModel::span(index);
}

/*!
    \property QRangeModel::supportedDragActions
    \since 6.12
    \brief the drag-actions supported by this model.

    Actions that the model cannot support, such as moving data out of a
    read-only model, will be removed when setting the actions.

    \note Overriding this function in a QRangeModel subclass is possible,
    but might break the behavior of the property.

    \sa supportedDropActions
*/
Qt::DropActions QRangeModel::supportedDragActions() const
{
    Q_D(const QRangeModel);
    return d->m_supportedDragActions;
}

void QRangeModel::setSupportedDragActions(Qt::DropActions actions)
{
    Q_D(QRangeModel);
    actions = d->impl->call<QRangeModelImplBase::AdjustSupportedDragActions>(actions);
    if (actions == d->m_supportedDragActions)
        return;
    d->m_supportedDragActions = actions;
    Q_EMIT supportedDragActionsChanged(d->m_supportedDragActions);
}

void QRangeModel::resetSupportedDragActions()
{
    setSupportedDragActions(Qt::CopyAction);
}

/*!
    \property QRangeModel::supportedDropActions
    \since 6.12
    \brief the drop-actions supported by this model.

    Read-only models cannot support any drop-actions.

    \note Overriding this function in a QRangeModel subclass is possible,
    but might break the behavior of the property.

    \sa supportedDragActions
*/
Qt::DropActions QRangeModel::supportedDropActions() const
{
    Q_D(const QRangeModel);
    return d->m_supportedDropActions;
}

void QRangeModel::setSupportedDropActions(Qt::DropActions actions)
{
    Q_D(QRangeModel);
    actions = d->impl->call<QRangeModelImplBase::AdjustSupportedDropActions>(actions);
    if (actions == d->m_supportedDropActions)
        return;
    d->m_supportedDropActions = actions;
    Q_EMIT supportedDropActionsChanged(d->m_supportedDropActions);
}

void QRangeModel::resetSupportedDropActions()
{
    setSupportedDropActions(Qt::CopyAction);
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
