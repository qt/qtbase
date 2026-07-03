// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tst_qrangemodel.h"

#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qstringlistmodel.h>
#include <QtTest/qsignalspy.h>

#include <QtGui/qcolor.h>
#include <QtGui/qpolygon.h>
#include <QtGui/qpen.h>

#if QT_CONFIG(itemmodeltester)
#include <QtTest/qabstractitemmodeltester.h>
#endif

#if defined(__cpp_lib_ranges)
#include <ranges>
#endif

QList<QPersistentModelIndex> tst_QRangeModel::allIndexes(QAbstractItemModel *model,
                                                         const QModelIndex &parent)
{
    QList<QPersistentModelIndex> pmiList;
    for (int row = 0; row < model->rowCount(parent); ++row) {
        for (int column = 0; column < model->columnCount(parent); ++column) {
            const QModelIndex mi = model->index(row, column, parent);
            pmiList += mi;
            if (model->hasChildren(mi))
                pmiList += allIndexes(model, mi);
        }
    }
    return pmiList;
}

void tst_QRangeModel::verifyPmiList(const QList<QPersistentModelIndex> &pmiList)
{
    for (const auto &pmi : pmiList) {
        auto debug = qScopeGuard([&pmi]{
            qCritical() << "Failing index" << pmi << pmi.isValid();
        });
        QVERIFY(pmi.isValid());
        QCOMPARE_NE(pmi.flags(), Qt::NoItemFlags);
        QCOMPARE(pmi.parent().isValid(), pmi.parent().data().isValid());
        debug.dismiss();
    }
}

void tst_QRangeModel::basics()
{
#if QT_CONFIG(itemmodeltester)
    QFETCH(Factory, factory);
    auto model = factory();

    QAbstractItemModelTester modelTest(model.get(), this);
#else
    QSKIP("QAbstractItemModelTester not available");
#endif
}

using ModelFromData = std::function<std::unique_ptr<QAbstractItemModel>(std::vector<int> &)>;

void tst_QRangeModel::modifies_data()
{
    QTest::addColumn<ModelFromData>("modelFromData");
    QTest::addColumn<bool>("modifiesOriginal");

    QTest::newRow("copy") << ModelFromData([](std::vector<int> &numbers){
        return std::unique_ptr<QAbstractItemModel>(new QRangeModel(numbers));
    }) << false;

    QTest::newRow("reference_wrapper") << ModelFromData([](std::vector<int> &numbers){
        return std::unique_ptr<QAbstractItemModel>(new QRangeModel(std::ref(numbers)));
    }) << true;

    QTest::newRow("pointer") << ModelFromData([](std::vector<int> &numbers){
        return std::unique_ptr<QAbstractItemModel>(new QRangeModel(&numbers));
    }) << true;
}

void tst_QRangeModel::modifies()
{
    QFETCH(ModelFromData, modelFromData);
    QFETCH(bool, modifiesOriginal);

    int dataSize = 1;
    std::vector<int> numbers { 1 };
    auto model = modelFromData(numbers);

    {
        QCOMPARE(model->rowCount(), numbers.size());
        const QModelIndex index = model->index(model->rowCount() - 1, 0);
        QCOMPARE(index.data(), numbers[index.row()]);
    }

    {
        QVERIFY(model->insertRows(0, 1));
        QCOMPARE(model->rowCount(), ++dataSize);
        QCOMPARE(int(numbers.size()) == model->rowCount(), modifiesOriginal);
    }

    {
        const QModelIndex index = model->index(0, 0);
        QVERIFY(model->setData(index, 2));
        QCOMPARE(index.data() == numbers[index.row()], modifiesOriginal);
    }
}

void tst_QRangeModel::minimalIterator()
{
    struct Minimal
    {
        struct iterator
        {
            using value_type = QString;
            using size_type = int;
            using difference_type = int;
            using reference = value_type;
            using pointer = value_type;
            using iterator_category = std::forward_iterator_tag;

            constexpr iterator &operator++()
            { ++m_index; return *this; }
            constexpr iterator operator++(int)
            { auto copy = *this; ++m_index; return copy; }

            reference operator*() const
            { return QString::number(m_index); }
            constexpr bool operator==(const iterator &other) const noexcept
            { return m_index == other.m_index; }
            constexpr bool operator!=(const iterator &other) const noexcept
            { return m_index != other.m_index; }

            size_type m_index;
        };

#if defined (__cpp_concepts)
        static_assert(std::forward_iterator<iterator>);
#endif
        iterator begin() const { return iterator{0}; }
        iterator end() const { return iterator{m_size}; }

        int m_size;
    } minimal{100};

    QRangeModel model(minimal);
    QCOMPARE(model.rowCount(), minimal.m_size);
    for (int row = model.rowCount() - 1; row >= 0; --row) {
        const QModelIndex index = model.index(row, 0);
        QCOMPARE(index.data(), QString::number(row));
        QVERIFY(!index.flags().testFlag(Qt::ItemIsEditable));
    }
}

void tst_QRangeModel::ranges()
{
#if defined(__cpp_lib_ranges)
    const int lowest = 1;
    const int highest = 10;
    QRangeModel model(std::views::iota(lowest, highest));
    QCOMPARE(model.rowCount(), highest - lowest);
    QCOMPARE(model.columnCount(), 1);
#else
    QSKIP("C++ ranges library not available");
#endif
}

void tst_QRangeModel::json()
{
    QJsonDocument json = QJsonDocument::fromJson(R"([ "one", "two" ])");
    QVERIFY(json.isArray());
    QRangeModel model(json.array());
    QCOMPARE(model.rowCount(), 2);
    const QModelIndex index = model.index(1, 0);
    QVERIFY(index.isValid());
    QCOMPARE(index.data().toString(), "two");
}

void tst_QRangeModel::ownership()
{
#if 0 // static assert expected
    {
        std::vector<std::vector<int> *> data;
        QRangeModel modelOnCopy(data);
    }

    {
        std::shared_ptr<std::vector<std::vector<int>*>> data;
        QRangeModel modelOnMovedData(std::move(data));
    }

    {
        QSharedPointer<std::vector<std::vector<int>*>> data;
        QRangeModel modelOnMovedData(std::move(data));
    }
#endif

    { // a list of pointers to objects
        Object *object = new Object;
        QPointer guard = object;
        std::vector<Object *> objects {
            object
        };
        { // model takes ownership of its own copy of the vector (!)
            QRangeModel modelOnMove(std::move(objects));
        }
        QVERIFY(!guard);
        objects = { new Object };
        guard = objects[0];
        { // model does not take ownership
            QRangeModel modelOnPointer(&objects);
        }
        QVERIFY(guard);
        { // model does not take ownership
            QRangeModel modelOnRef(std::ref(objects));
        }
        QVERIFY(guard);

        { // model does take ownership
            QRangeModel movedIntoModel(std::move(objects));
            QCOMPARE(movedIntoModel.columnCount(), 2);
        }
        QVERIFY(!guard);
    }

    { // a list of shared_ptr
        Object *object = new Object;
        QPointer guard = object;
        std::vector<std::shared_ptr<Object>> objects {
            std::shared_ptr<Object>(object)
        };
        { // model does not take ownership
            QCOMPARE(objects[0].use_count(), 1);
            QRangeModel modelOnCopy(objects);
            QCOMPARE(modelOnCopy.rowCount(), 1);
            QCOMPARE(objects[0].use_count(), 2);
        }
        QCOMPARE(objects[0].use_count(), 1);
        { // model does not take ownership
            QRangeModel modelOnPointer(&objects);
            QCOMPARE(objects[0].use_count(), 1);
        }
        QCOMPARE(objects[0].use_count(), 1);
        QVERIFY(guard);
        { // model does not take ownership
            QRangeModel modelOnRef(std::ref(objects));
            QCOMPARE(objects[0].use_count(), 1);
        }
        QCOMPARE(objects[0].use_count(), 1);
        QVERIFY(guard);
        { // model owns the last shared copy
            QRangeModel movedIntoModel(std::move(objects));
        }
        QVERIFY(!guard);
    }

    { // a table of pointers
        Object *object = new Object;
        QPointer guard = object;
        std::vector<std::vector<Object *>> table {
            {object}
        };
        { // model does not take ownership
            QRangeModel modelOnCopy(table);
        }
        QVERIFY(guard);
        { // model does not take ownership
            QRangeModel modelOnPointer(&table);
        }
        QVERIFY(guard);
        { // model does not take ownership
            QRangeModel modelOnRef(std::ref(table));
        }
        QVERIFY(guard);
        { // model does take ownership of rows, but not of objects within each row
            QRangeModel movedIntoModel(std::move(table));
        }
        QVERIFY(guard);
        delete object;
    }

    { // a table of shared pointers to rows
        std::vector<std::shared_ptr<Object>> objects = { std::make_shared<Object>() };

        {
            QRangeModel model(objects);
            QCOMPARE(objects.front().use_count(), 2);
        }

        QCOMPARE(objects.front().use_count(), 1);
    }

    { // a table of shared pointers to rows
        using SharedObjectsList = std::vector<std::shared_ptr<Object>>;
        std::vector<std::shared_ptr<SharedObjectsList>> table = {
            std::make_shared<SharedObjectsList>(SharedObjectsList{ std::make_shared<Object>() })
        };

        {
            QRangeModel model(table);
            QCOMPARE(table.front().use_count(), 2);
            QCOMPARE(table.front()->front().use_count(), 1);
        }

        QCOMPARE(table.front().use_count(), 1);
        QCOMPARE(table.front()->front().use_count(), 1);
    }
}

void tst_QRangeModel::overrideRoleNames()
{
    // verify that an overridden roleNames() gets called consistently
    class RoleModel : public QRangeModel
    {
    public:
        RoleModel() : QRangeModel(QList<std::tuple<Object *>>{
            new Object,
            new Object,
            new Object,
        }) {
        }

        QHash<int, QByteArray> roleNames() const override
        {
            return {
                {Qt::UserRole, "string"},
                {Qt::UserRole + 1, "number"}
            };
        }
    };

    RoleModel model;
    const QList<int> expectedKeys = {Qt::UserRole, Qt::UserRole + 1};
    QCOMPARE(model.roleNames().size(), expectedKeys.size());

    const QModelIndex index = model.index(0, 0);
    QVERIFY(model.setData(index, "string value", Qt::UserRole));
    QVERIFY(model.setData(index, 42, Qt::UserRole + 1));
    QVERIFY(!model.setData(index, "display"));

    const auto itemData = model.itemData(index);
    QCOMPARE(itemData.keys(), expectedKeys);
    QCOMPARE(itemData.value(Qt::UserRole), "string value");
    QCOMPARE(itemData.value(Qt::UserRole + 1), 42);

    QVERIFY(model.setItemData(model.index(1, 0), itemData));
}

void tst_QRangeModel::setRoleNames()
{
    QRangeModel model(QStringList{});

    const QHash<int, QByteArray> expectedRoleNames = {
        {Qt::DisplayRole, "display"},
        {Qt::EditRole, "edit"},
        {Qt::RangeModelDataRole, "modelData"},
    };

    QSignalSpy spy(&model, &QRangeModel::roleNamesChanged);
    QCOMPARE(model.roleNames(), expectedRoleNames);
    QVERIFY(spy.isEmpty());

    const QHash<int, QByteArray> roleNames = {
        {Qt::UserRole, "one"},
        {Qt::UserRole + 1, "two"},
    };
    model.setRoleNames(roleNames);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(model.roleNames(), roleNames);

    model.setRoleNames({});
    QCOMPARE(spy.count(), 2);
    QCOMPARE(model.roleNames(), expectedRoleNames);
}

void tst_QRangeModel::defaultRoleNames()
{
    // default QAIM role names for anything that we didn't specialize roleNames for
    const QHash<int, QByteArray> qaimRoleNames = QStringListModel().roleNames();

    [qaimRoleNames]{
        const QHash<int, QByteArray> expectedRoleNames = {
            {Qt::RangeModelDataRole, "modelData"},
            {Qt::UserRole, "string"},
            {Qt::UserRole + 1, "number"},
        };

        QCOMPARE(QRangeModel(QList<Object *>{}).roleNames(),
                 qaimRoleNames);
        QCOMPARE(QRangeModel(QList<std::tuple<Object *>>{}).roleNames(),
                 expectedRoleNames);
        QCOMPARE(QRangeModel(QList<std::tuple<Object *, Object *>>{}).roleNames(),
                 expectedRoleNames);
    }();

    [qaimRoleNames]{
        const QHash<int, QByteArray> expectedRoleNames = {
            {Qt::RangeModelDataRole, "modelData"},
            {Qt::DisplayRole, "display"},
            {Qt::DecorationRole, "decoration"},
            {Qt::ToolTipRole, "toolTip"},
        };
        QCOMPARE(QRangeModel(QList<Item>{}).roleNames(),
                 qaimRoleNames);
        QCOMPARE(QRangeModel(QList<std::tuple<Item>>{}).roleNames(),
                 expectedRoleNames);
        QCOMPARE(QRangeModel(QList<std::tuple<Item, Item, Item>>{}).roleNames(),
                 expectedRoleNames);
        QCOMPARE(QRangeModel(QList<QList<Item>>{}).roleNames(),
                 expectedRoleNames);
    }();

    [qaimRoleNames]{
        using Tree = QList<MultiRoleGadget>;
        struct EmptyTreeProtocol
        {
            const MultiRoleGadget *parentRow(const MultiRoleGadget &) const { return nullptr; }
            const Tree &childRows(const MultiRoleGadget &) const { return empty; }
            Tree empty;
        };
        const QHash<int, QByteArray> expectedRoleNames = {
            {Qt::RangeModelDataRole, "modelData"},
            {Qt::DisplayRole, "display"},
            {Qt::DecorationRole, "decoration"},
            {Qt::UserRole, "number"},
            {Qt::UserRole + 1, "user"},
        };
        QCOMPARE(QRangeModel(QList<MultiRoleGadget>{}).roleNames(),
                 expectedRoleNames);
        QCOMPARE(QRangeModel(QList<QList<MultiRoleGadget>>{}).roleNames(),
                 expectedRoleNames);
        QCOMPARE(QRangeModel(std::vector<std::array<MultiRoleGadget, 5>>{}).roleNames(),
                 expectedRoleNames);
        QCOMPARE(QRangeModel(Tree{}, EmptyTreeProtocol{}).roleNames(),
                 expectedRoleNames);
    }();

    [qaimRoleNames]{
        const QHash<int, QByteArray> singleValueRoleNames = {
            {Qt::DisplayRole, "display"},
            {Qt::EditRole, "edit"},
            {Qt::RangeModelDataRole, "modelData"},
        };

        QCOMPARE(QRangeModel(QList<Row>{}).roleNames(), qaimRoleNames);
        QCOMPARE(QRangeModel(QList<std::tuple<Item, MultiRoleGadget>>{}).roleNames(),
                 qaimRoleNames);
        QCOMPARE(QRangeModel(QList<int>{}).roleNames(), singleValueRoleNames);
        QCOMPARE(QRangeModel(QList<QList<QString>>{}).roleNames(), singleValueRoleNames);
    }();
}

class MultiRoleObject : public Object
{
public:
    template <typename Signal>
    bool isConnected(Signal &&signal) const
    {
        return isSignalConnected(QMetaMethod::fromSignal(signal));
    }
};

template <>
struct QRangeModel::RowOptions<MultiRoleObject>
{
    static constexpr auto rowCategory = QRangeModel::RowCategory::MultiRoleItem;
};

void tst_QRangeModel::autoConnectPolicy_data()
{
    QTest::addColumn<QRangeModel::AutoConnectPolicy>("policy");

    QTest::addRow("Full") << QRangeModel::AutoConnectPolicy::Full;
    QTest::addRow("OnRead") << QRangeModel::AutoConnectPolicy::OnRead;
}

void tst_QRangeModel::autoConnectPolicy()
{
    QFETCH(const QRangeModel::AutoConnectPolicy, policy);

    [policy]{
        QList<MultiRoleObject *> objectList = {
            new MultiRoleObject,
            new MultiRoleObject,
            new MultiRoleObject,
        };
        QRangeModel model(&objectList);
        model.setAutoConnectPolicy(policy);
        QSignalSpy dataChangedSpy(&model, &QAbstractItemModel::dataChanged);

        int emissions = 0;
        objectList[0]->setString("String 0");
        if (policy == QRangeModel::AutoConnectPolicy::OnRead) {
            QCOMPARE(dataChangedSpy.size(), emissions);
        } else {
            QCOMPARE(dataChangedSpy.size(), ++emissions);
            QCOMPARE(dataChangedSpy.at(0).at(0), model.index(0, 0));
            QCOMPARE(dataChangedSpy.at(0).at(1), model.index(0, 0));
            QCOMPARE(dataChangedSpy.at(0).at(2), QVariant::fromValue(QList<int>{Qt::UserRole}));
        }

        if (policy == QRangeModel::AutoConnectPolicy::OnRead) {
            QVERIFY(!objectList.at(1)->isConnected(&Object::stringChanged));
            QVERIFY(!objectList.at(1)->isConnected(&Object::numberChanged));
            model.data(model.index(1, 0), Qt::UserRole + 1);
            QVERIFY(!objectList.at(1)->isConnected(&Object::stringChanged));
            QVERIFY(objectList.at(1)->isConnected(&Object::numberChanged));
            model.itemData(model.index(1, 0));
            QVERIFY(objectList.at(1)->isConnected(&Object::stringChanged));
        }

        objectList[1]->setNumber(42);
        QCOMPARE(dataChangedSpy.size(), ++emissions);
        QCOMPARE(dataChangedSpy.at(emissions - 1).at(0), model.index(1, 0));
        QCOMPARE(dataChangedSpy.at(emissions - 1).at(1), model.index(1, 0));
        QCOMPARE(dataChangedSpy.at(emissions - 1).at(2), QVariant::fromValue(QList<int>{Qt::UserRole + 1}));

        QVERIFY(model.insertRow(0));
        QCOMPARE(objectList.at(1)->isConnected(&Object::numberChanged),
                 policy == QRangeModel::AutoConnectPolicy::Full);
        QCOMPARE(objectList.at(1)->isConnected(&Object::stringChanged),
                 policy == QRangeModel::AutoConnectPolicy::Full);
    }();

    [policy]{
        QList<QList<MultiRoleObject *>> objectTable = {
            {new MultiRoleObject, new MultiRoleObject},
            {new MultiRoleObject, new MultiRoleObject},
        };
        QRangeModel model(&objectTable);
        connect(&model, &QRangeModel::rowsInserted,
                &model, [&objectTable](const QModelIndex &, int first, int last){
            while (first <= last) {
                objectTable[first][0] = new MultiRoleObject;
                objectTable[first][1] = new MultiRoleObject;
                ++first;
            }
        });
        connect(&model, &QRangeModel::columnsInserted,
                &model, [&objectTable](const QModelIndex &, int first, int last){
            for (auto &row : objectTable) {
                for (int column = first; column <= last; ++column)
                    row[column] = new MultiRoleObject;
            }
        });
        model.setAutoConnectPolicy(policy);
        QSignalSpy dataChangedSpy(&model, &QAbstractItemModel::dataChanged);

        objectTable[0][1]->setString("String 0/1");
        QCOMPARE(dataChangedSpy.size(), policy == QRangeModel::AutoConnectPolicy::Full ? 1 : 0);

        model.insertRows(1, 2);
        for (const auto &row : std::as_const(objectTable)) {
            for (const auto &object : row) {
                QCOMPARE(object->isConnected(&Object::numberChanged),
                         policy == QRangeModel::AutoConnectPolicy::Full);
                QCOMPARE(object->isConnected(&Object::stringChanged),
                         policy == QRangeModel::AutoConnectPolicy::Full);
            }
        }

        model.insertColumn(0);
        for (const auto &row : std::as_const(objectTable)) {
            for (const auto &object : row) {
                QCOMPARE(object->isConnected(&Object::numberChanged),
                         policy == QRangeModel::AutoConnectPolicy::Full);
                QCOMPARE(object->isConnected(&Object::stringChanged),
                         policy == QRangeModel::AutoConnectPolicy::Full);
            }
        }
    }();

    [policy]{
        QList<std::tuple<MultiRoleObject *, MultiRoleObject *>> objectTable = {
            {new MultiRoleObject, new MultiRoleObject},
            {new MultiRoleObject, new MultiRoleObject},
        };
        QRangeModel model(&objectTable);
        model.setAutoConnectPolicy(policy);
        QSignalSpy dataChangedSpy(&model, &QAbstractItemModel::dataChanged);

        if (policy == QRangeModel::AutoConnectPolicy::OnRead) {
            for (int row = 0; row < model.rowCount(); ++row) {
                for (int column = 0; column < model.columnCount(); ++column)
                    model.itemData(model.index(row, column));
            }
        }

        auto *topRight = get<1>(objectTable[0]);
        auto *bottomLeft = get<0>(objectTable[1]);
        topRight->setNumber(52);
        QCOMPARE(dataChangedSpy.size(), 1);

        QVERIFY(bottomLeft->isConnected(&Object::numberChanged));
        QVERIFY(bottomLeft->isConnected(&Object::stringChanged));
        QVERIFY(model.removeRows(1, 1));
        bottomLeft->setNumber(52); // this will lazily break the connection
        QVERIFY(!bottomLeft->isConnected(&Object::numberChanged));
        QVERIFY(bottomLeft->isConnected(&Object::stringChanged));
        QCOMPARE(dataChangedSpy.size(), 1);
        bottomLeft->setNumber(53); // this should not crash
        bottomLeft->setString("No update");
        QVERIFY(!bottomLeft->isConnected(&Object::stringChanged));

        const QModelIndex index = model.index(0, 0);
        dataChangedSpy.clear();
        QVERIFY(model.setData(index, "string", Qt::UserRole));
        QCOMPARE(dataChangedSpy.count(), 1);
        QCOMPARE(dataChangedSpy.back().at(2),
                 QVariant::fromValue(QList<int>{Qt::UserRole}));
        // this will right now emit dataChanged three times:
        QVERIFY(model.setItemData(index, QMap<int, QVariant>{
            {Qt::UserRole, QVariant("string")},
            {Qt::UserRole + 1, QVariant(42)},
        }));
        QCOMPARE(dataChangedSpy.count(), 2);
        QCOMPARE(dataChangedSpy.back().at(2),
                 QVariant::fromValue(QList<int>{Qt::UserRole, Qt::UserRole + 1}));
        QVERIFY(model.setData(index, 42, Qt::UserRole + 1));
        QCOMPARE(dataChangedSpy.count(), 3);
        QCOMPARE(dataChangedSpy.back().at(2),
                 QVariant::fromValue(QList<int>{Qt::UserRole + 1}));
    }();

    [policy]{
        QList<Object *> objectList = {
            new Object, new Object, new Object
        };
        Object *top = objectList.front();
        Object *bottom = objectList.back();

        QRangeModel model(std::move(objectList));
        model.setAutoConnectPolicy(policy);
        QSignalSpy dataChangedSpy(&model, &QAbstractItemModel::dataChanged);

        if (policy == QRangeModel::AutoConnectPolicy::OnRead) {
            // read top-left and bottom-right
            model.data(model.index(0, 0));
            model.data(model.index(model.rowCount() - 1, model.columnCount() - 1));
        }
        top->setString("abc");
        QCOMPARE(dataChangedSpy.count(), 1);
        QCOMPARE(dataChangedSpy.back().at(0), model.index(0, 0));
        QCOMPARE(dataChangedSpy.back().at(1), model.index(0, 0));
        QCOMPARE(dataChangedSpy.back().at(2), QVariant::fromValue(QList<int>{Qt::DisplayRole}));
        bottom->setNumber(42);
        QCOMPARE(dataChangedSpy.count(), 2);
        QCOMPARE(dataChangedSpy.back().at(0), model.index(model.rowCount() - 1,
                                                          model.columnCount() - 1));
        QCOMPARE(dataChangedSpy.back().at(2), QVariant::fromValue(QList<int>{Qt::DisplayRole}));
        dataChangedSpy.clear();

        top->setNumber(1234);
        bottom->setString("def");
        QCOMPARE(dataChangedSpy.count(), policy == QRangeModel::AutoConnectPolicy::Full ? 2 : 0);
    }();

    [policy]{
        using Tree = QList<MultiRoleObject *>;
        struct ObjectTreeProtocol
        {
            const MultiRoleObject *parentRow(const MultiRoleObject &row) const
            {
                return static_cast<MultiRoleObject *>(row.parent());
            }
            void setParentRow(MultiRoleObject &row, MultiRoleObject *parent)
            {
                row.setParent(parent);
            }
            const Tree &childRows(const MultiRoleObject &row) const {
                // don't do that at home...
                return *reinterpret_cast<const Tree *>(&row.children());
            }
            Tree &childRows(const MultiRoleObject &) {
                empty = {};
                return empty;
            }
            Tree empty;
        };
        Tree tree {
            new MultiRoleObject,
            new MultiRoleObject,
            new MultiRoleObject,
        };
        tree[0]->setObjectName("root 0");
        tree[1]->setObjectName("root 1");
        tree[2]->setObjectName("root 2");
        auto *child01 = new MultiRoleObject;
        child01->setObjectName("child 0/1");
        child01->setParent(tree[0]);
        (new MultiRoleObject)->setParent(tree[1]);
        (new MultiRoleObject)->setParent(tree[2]);

        QRangeModel model(std::ref(tree), ObjectTreeProtocol{});
        QSignalSpy dataChangedSpy(&model, &QAbstractItemModel::dataChanged);
        model.setAutoConnectPolicy(policy);

        const QModelIndex root0 = model.index(0, 0);
        const QModelIndex child01Index = model.index(0, 0, root0);
        if (policy == QRangeModel::AutoConnectPolicy::OnRead)
            QVERIFY(model.data(child01Index, Qt::UserRole + 1).isValid());

        child01->setNumber(42);
        QCOMPARE(dataChangedSpy.size(), 1);

        QCOMPARE(dataChangedSpy.at(0).at(0).value<QModelIndex>(), child01Index);
        QCOMPARE(dataChangedSpy.at(0).at(1).value<QModelIndex>(), child01Index);
        QCOMPARE(dataChangedSpy.at(0).at(2), QVariant::fromValue(QList{Qt::UserRole + 1}));
    }();

    // build tests
    { // make sure we don't kill the compiler with recursive templates
        QList<std::array<Object *, 1000000>> wideList;
        QRangeModel model(wideList);
    }

    { // work with custom tuple types
        QList<ObjectRow> objectRows;
        QRangeModel model(objectRows);
    }

    { // correctly resolve optional children
        struct Protocol {
            ObjectRow *parentRow(const ObjectRow &) const { return nullptr; }
            const auto &childRows(const ObjectRow &) const { return emptyRow; }

            std::optional<std::vector<ObjectRow>> emptyRow = std::nullopt;
        };
        std::vector<ObjectRow> objectTree;
        QRangeModel model(objectTree, Protocol{});
        model.setAutoConnectPolicy(QRangeModel::AutoConnectPolicy::Full);
    }
}

void tst_QRangeModel::dimensions()
{
    QFETCH(Factory, factory);
    auto model = factory();
    QFETCH(const int, expectedRowCount);
    QFETCH(const int, expectedColumnCount);

    QCOMPARE(model->rowCount(), expectedRowCount);
    QCOMPARE(model->columnCount(), expectedColumnCount);
}

void tst_QRangeModel::sibling()
{
    QFETCH(Factory, factory);
    auto model = factory();

    QModelIndex withChildren;
    const auto test = [model = model.get(), &withChildren](const QModelIndex &parent){
        const QModelIndex first = model->index(0, 0, parent);
        // deliberately requesting siblings outside of the range
        for (int r = 0; r < model->rowCount() + 1; ++r) {
            for (int c = 0; c < model->columnCount() + 1; ++c) {
                const QModelIndex next = model->sibling(r, c, first);
                const QModelIndex qaimNext = model->QAbstractItemModel::sibling(r, c, first);
                if (!withChildren.isValid() && model->hasChildren(next))
                    withChildren = next;
                QCOMPARE(next, qaimNext);
            }
        }
    };

    test({});
    if (withChildren.isValid())
        test(withChildren);
}

void tst_QRangeModel::flags()
{
    QFETCH(Factory, factory);
    auto model = factory();
    QFETCH(const ChangeActions, changeActions);

    const bool hasMimeTypes = !model->mimeTypes().isEmpty();
    const QModelIndex first = model->index(0, 0);
    QVERIFY(first.isValid());
    const QModelIndex last = model->index(model->rowCount() - 1, model->columnCount() - 1);
    QVERIFY(last.isValid());

    QCOMPARE(first.flags().testFlag(Qt::ItemIsEditable),
             changeActions.testFlags(ChangeAction::SetData));
    QCOMPARE(last.flags().testFlag(Qt::ItemIsEditable),
             changeActions.testFlags(ChangeAction::SetData));
    if (last.column() != 0)
        QVERIFY(last.flags().testFlag(Qt::ItemNeverHasChildren));
    QCOMPARE(first.flags().testFlag(Qt::ItemIsDragEnabled), hasMimeTypes);
    QCOMPARE(first.flags().testFlag(Qt::ItemIsDropEnabled),
             changeActions.testAnyFlags(ChangeAction::SetData | ChangeAction::InsertRows));
}

void tst_QRangeModel::headerData()
{
    QFETCH(Factory, factory);
    QFETCH(QVariant, headerValue);
    auto model = factory();

    QCOMPARE(model->headerData(0, Qt::Horizontal), headerValue);
}

void tst_QRangeModel::data()
{
    QFETCH(Factory, factory);
    auto model = factory();

    QVERIFY(!model->data({}).isValid());

    const QModelIndex first = model->index(0, 0);
    QVERIFY(first.isValid());
    const QModelIndex last = model->index(model->rowCount() - 1, model->columnCount() - 1);
    QVERIFY(last.isValid());

    QVERIFY(first.data().isValid());
    QVERIFY(last.data().isValid());
}

void tst_QRangeModel::multiData()
{
    QFETCH(Factory, factory);
    auto model = factory();

    const QModelIndex index = model->index(0, 0);
    QVERIFY(index.isValid());
    QModelRoleData displayData(Qt::DisplayRole);
    model->multiData(index, displayData);

    QCOMPARE(displayData.data(), model->data(index, Qt::DisplayRole));
}

void tst_QRangeModel::setData()
{
    QFETCH(Factory, factory);
    auto model = factory();
    QFETCH(const ChangeActions, changeActions);

    QVERIFY(!model->setData({}, {}));

    const QModelIndex first = model->index(0, 0);
    QVERIFY(first.isValid());

    QVariant newValue = 12345;
    const QVariant oldValue = first.data();
    QVERIFY(oldValue.isValid());

    if (!newValue.canConvert(oldValue.metaType()))
        newValue = QVariant(oldValue.metaType());
    QCOMPARE(first.data(), oldValue);
    QCOMPARE(model->setData(first, newValue), changeActions.testFlag(ChangeAction::SetData));
    QCOMPARE(first.data() == oldValue, !changeActions.testFlag(ChangeAction::SetData));

    // don't crash for invalid role values, but ignore return value - it will
    // work with items that are backed by a map.
    model->setData(first, oldValue, Qt::UserRole + 255);
}

static constexpr bool fakedRole(int role)
{
    return role == Qt::EditRole
        || role == Qt::RangeModelDataRole
        || role == Qt::RangeModelDataRole + 1;
}

void tst_QRangeModel::itemData()
{
    QFETCH(Factory, factory);
    auto model = factory();

    QVERIFY(model->itemData({}).isEmpty());

    const QModelIndex index = model->index(0, 0);
    const QMap<int, QVariant> itemData = model->itemData(index);
    for (int role = 0; role < Qt::UserRole; ++role) {
        // we fake those in data()
        if (fakedRole(role))
            continue;
        QCOMPARE(itemData.value(role), index.data(role));
    }
}

void tst_QRangeModel::setItemData()
{
    QFETCH(Factory, factory);
    auto model = factory();
    QFETCH(const ChangeActions, changeActions);

    QVERIFY(!model->setItemData({}, {}));

    const QModelIndex index = model->index(0, 0);
    QMap<int, QVariant> itemData = model->itemData(index);
    // we only care about multi-role models
    const auto roles = itemData.keys();
    if (roles == QList<int>{Qt::DisplayRole, Qt::EditRole}
     || roles == QList<int>{Qt::DisplayRole, Qt::EditRole, Qt::RangeModelDataRole}) {
        QSKIP("Can't test setItemData on models with single values!");
     }

    itemData = {};
    for (int role : roles) {
        if (fakedRole(role)) // faked
            continue;
        QVariant data = role != Qt::DecorationRole ? QVariant(QStringLiteral("%1").arg(role))
                                                   : QVariant(QColor(Qt::magenta));
        itemData.insert(role, data);
    }

    QCOMPARE_NE(model->itemData(index), itemData);
    QCOMPARE(model->setItemData(index, itemData),
             changeActions.testFlag(ChangeAction::SetItemData));
    if (!changeActions.testFlag(ChangeAction::SetItemData))
        return; // nothing more to test for those models

    {
        auto newItemData = model->itemData(index);
        newItemData.take(Qt::EditRole); // faked
        auto diagnostics = qScopeGuard([&]{
            qDebug() << "Mismatch";
            qDebug() << "     Actual:" << newItemData;
            qDebug() << "   Expected:" << itemData;
        });
        QCOMPARE(newItemData == itemData, changeActions.testFlag(ChangeAction::SetItemData));
        diagnostics.dismiss();
    }

    for (int role = 0; role < Qt::UserRole; ++role) {
        if (fakedRole(role))
            continue;

        QVariant data = index.data(role);
        auto diagnostics = qScopeGuard([&]{
            qDebug() << "Mismatch for" << Qt::ItemDataRole(role);
            qDebug() << "     Actual:" << data;
            qDebug() << "   Expected:" << itemData.value(role);
        });
        QCOMPARE(data == itemData.value(role), changeActions.testFlag(ChangeAction::SetData));
        diagnostics.dismiss();
    }
}

void tst_QRangeModel::clearItemData()
{
    QFETCH(Factory, factory);
    auto model = factory();
    QFETCH(const ChangeActions, changeActions);

    QVERIFY(!model->clearItemData({}));

    const QModelIndex index0 = model->index(1, 0);
    const QModelIndex index1 = model->index(1, 1);
    const QVariant oldDataAt0 = index0.data();
    const QVariant oldDataAt1 = index1.data();
    QCOMPARE(model->clearItemData(index0), changeActions.testFlags(ChangeAction::SetData));
    QCOMPARE(index0.data() == oldDataAt0, !changeActions.testFlags(ChangeAction::SetData));
    QCOMPARE(index1.data(), oldDataAt1);
}

void tst_QRangeModel::modelData()
{
    QFETCH(Factory, factory);
    auto model = factory();
    QFETCH(const ChangeActions, changeActions);

    const auto roleNames = model->roleNames();
    // models must support RangeModelDataRole if it's part of roleNames;
    // otherwise, we still might support it for certain columns.
    const bool promisesRangeModelData = roleNames.contains(Qt::RangeModelDataRole);
    const QModelIndex index = model->index(0, 0);
    const QVariant data = model->data(index, Qt::RangeModelDataRole);
    QVERIFY(data.isValid() || !promisesRangeModelData);

    bool setDataResult = false;
    // we can not swap out QObjects, even if setData() is permitted and
    // RangeModelDataRole is reported
    if (changeActions.testFlag(ChangeAction::SetData) && data.isValid()) {
        QEXPECT_FAIL("listOfMetaObjectTupleCopy", "Can't replace QObject items", Continue);
        QEXPECT_FAIL("arrayOfUniqueMultiObjectTuplesRef", "Can't replace QObject items", Continue);
        setDataResult = model->setData(index, data, Qt::RangeModelDataRole);
        QVERIFY(setDataResult || !promisesRangeModelData);
        if (setDataResult) {
            // if we could setData (with an unchanged value), then try with a
            // different row, and verify that the DisplayRole changes.
            if (model->rowCount() > 1) {
                const QModelIndex index2 = model->index(1, 0);
                const QVariant data2 = model->data(index2, Qt::RangeModelDataRole);
                QVERIFY(model->setData(index, data2, Qt::RangeModelDataRole));
                QCOMPARE(model->data(index, Qt::DisplayRole), model->data(index2, Qt::DisplayRole));
            } else {
                QSKIP("Cannot test changing of modelData with a model with only one row");
            }
        }
    }
}

void tst_QRangeModel::rangeModelDataInTable()
{
    std::vector<Object *> table = {
        new Object,
        new Object,
        new Object
    };
    QRangeModel model(std::ref(table));
    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(model.columnCount(), 2);

    const QModelIndex topLeft = model.index(0, 0);
    const QModelIndex bottomRight = model.index(2, 1);
    QCOMPARE(model.data(topLeft), table.at(topLeft.row())->string());
    QCOMPARE(model.data(bottomRight), table.at(bottomRight.row())->number());

    QVERIFY(model.setData(topLeft, "fortyTwo", Qt::RangeModelDataRole));
    QVERIFY(model.setData(bottomRight, 42, Qt::RangeModelDataRole));
}

void tst_QRangeModel::insertRows()
{
    QFETCH(Factory, factory);
    auto model = factory();
    QFETCH(const int, expectedRowCount);
    QFETCH(const int, expectedColumnCount);
    QFETCH(const ChangeActions, changeActions);
    const bool canSetData = changeActions.testFlag(ChangeAction::SetData);

    // associative containers are empty, so we need to explicitly set data for
    // newly created rows
    static const QList<QByteArrayView> associativeContainers = {
        "listOfNamedRoles",
        "tableOfEnumRoles",
        "tableOfIntRoles",
        "stdTableOfIntRoles",
        "stdTableOfIntRolesWithSharedRows",
    };
    const auto it = std::find_if(associativeContainers.begin(), associativeContainers.end(),
                [current = QByteArrayView(QTest::currentDataTag())](const QByteArrayView &tag) {
        if (tag == current)
            return true;
        for (auto suffix : { "Pointer", "Copy", "Ref", "UPtr", "SPtr" }) {
            if (tag + suffix == current)
                return true;
        }
        return false;
    });

    if (it != associativeContainers.end()) {
        connect(model.get(), &QAbstractItemModel::rowsInserted,
                this, [model = model.get()](const QModelIndex &parent, int start, int end) {
            for (int row = start; row <= end; ++row) {
                model->setData(model->index(row, 0, parent), row);
                model->setData(model->index(row, model->columnCount(parent) - 1, parent), row);
            }
        });
    }

    const QList<QPersistentModelIndex> pmiList = allIndexes(model.get());

    QCOMPARE(model->rowCount(), expectedRowCount);
    QCOMPARE(model->insertRow(0), changeActions.testFlag(ChangeAction::InsertRows));
    QCOMPARE(model->rowCount() == expectedRowCount + 1,
             changeActions.testFlag(ChangeAction::InsertRows));

    // get and put data into the new row
    const QModelIndex firstItem = model->index(0, 0);
    const QModelIndex lastItem = model->index(0, expectedColumnCount - 1);
    QVERIFY(firstItem.isValid());
    QVERIFY(lastItem.isValid());
    const QVariant firstValue = firstItem.data();
    const QVariant lastValue = lastItem.data();

    QEXPECT_FAIL("tableOfPointersPointer", "No item created", Continue);
    QEXPECT_FAIL("listOfMetaObjectTupleCopy", "No object created", Continue);
    QEXPECT_FAIL("listOfMetaObjectTupleMoveOfCopy", "No object created", Continue);

    QVERIFY(firstValue.isValid() && lastValue.isValid());
    QCOMPARE(model->setData(firstItem, lastValue), canSetData && lastValue.isValid());
    QCOMPARE(model->setData(lastItem, firstValue), canSetData && firstValue.isValid());

    // append more rows
    QCOMPARE(model->insertRows(model->rowCount(), 5),
             changeActions.testFlag(ChangeAction::InsertRows));
    QCOMPARE(model->rowCount() == expectedRowCount + 6,
             changeActions.testFlag(ChangeAction::InsertRows));

    verifyPmiList(pmiList);
}

void tst_QRangeModel::removeRows()
{
    QFETCH(Factory, factory);
    auto model = factory();
    QFETCH(const int, expectedRowCount);
    QFETCH(const ChangeActions, changeActions);

    QCOMPARE(model->rowCount(), expectedRowCount);
    QCOMPARE(model->removeRow(0), changeActions.testFlag(ChangeAction::RemoveRows));
    QCOMPARE(model->rowCount() == expectedRowCount - 1,
             changeActions.testFlag(ChangeAction::RemoveRows));
    QCOMPARE(model->removeRows(model->rowCount() - 2, 2),
             changeActions.testFlag(ChangeAction::RemoveRows));
    QCOMPARE(model->rowCount() == expectedRowCount - 3,
             changeActions.testFlag(ChangeAction::RemoveRows));

    const int newRowCount = model->rowCount();
    // make sure we don't crash when removing more than exist
    const bool couldRemove = model->removeRows(model->rowCount() - 5, model->rowCount() * 2);
    QCOMPARE_LE(model->rowCount(), newRowCount);
    QCOMPARE(couldRemove, model->rowCount() != newRowCount);
}

void tst_QRangeModel::moveRows()
{
    QFETCH(Factory, factory);
    auto model = factory();
    QFETCH(const int, expectedRowCount);
    QFETCH(const ChangeActions, changeActions);

    QCOMPARE(model->rowCount(), expectedRowCount);
    if (expectedRowCount < 3)
        QSKIP("Model is to small for testing moveRows");

    const QVariant first = model->index(0, 0).data();
    const QVariant second = model->index(1, 0).data();
    const QVariant last = model->index(expectedRowCount - 1, 0).data();

    // various noops, should always fail
    QVERIFY(!model->moveRows({}, 0, 1, {}, 0));
    QVERIFY(!model->moveRows({}, 0, 1, {}, 1));
    QVERIFY(!model->moveRows({}, 0, 0, {}, expectedRowCount));

    // try to move first to last
    QCOMPARE(model->moveRows({}, 0, 1, {}, expectedRowCount),
             changeActions != ChangeAction::ReadOnly);
    if (changeActions == ChangeAction::ReadOnly)
        return;

    QCOMPARE(model->index(0, 0).data(), second); // second is now on first
    QCOMPARE(model->index(expectedRowCount - 2, 0).data(), last); // last is now second last
    QCOMPARE(model->index(expectedRowCount - 1, 0).data(), first);

    // move all but one row to the end - this restores the order
    QVERIFY(model->moveRows({}, 0, expectedRowCount - 1,
                            {}, expectedRowCount));
    QCOMPARE(model->index(0, 0).data(), first);
    QCOMPARE(model->index(1, 0).data(), second);
    QCOMPARE(model->index(expectedRowCount - 1, 0).data(), last);

    // move the last row step by step up to the top
    for (int row = model->rowCount() - 1; row > 0; --row)
        QVERIFY(model->moveRow({}, row, {}, row - 1));
    QCOMPARE(model->index(0, 0).data(), last);
    // move all except the first row up - this restores the order again
    QVERIFY(model->moveRows({}, 1, expectedRowCount - 1, {}, 0));
    QCOMPARE(model->index(0, 0).data(), first);
    QCOMPARE(model->index(1, 0).data(), second);
    QCOMPARE(model->index(expectedRowCount - 1, 0).data(), last);
}

void tst_QRangeModel::insertColumns()
{
    QFETCH(Factory, factory);
    auto model = factory();
    QFETCH(const int, expectedColumnCount);
    QFETCH(const ChangeActions, changeActions);

    QCOMPARE(model->columnCount(), expectedColumnCount);
    QCOMPARE(model->insertColumn(0), changeActions.testFlag(ChangeAction::InsertColumns));
    QCOMPARE(model->columnCount() == expectedColumnCount + 1,
             changeActions.testFlag(ChangeAction::InsertColumns));

    // append
    QCOMPARE(model->insertColumns(model->columnCount(), 5),
             changeActions.testFlag(ChangeAction::InsertColumns));
    QCOMPARE(model->columnCount() == expectedColumnCount + 6,
             changeActions.testFlag(ChangeAction::InsertColumns));
}

void tst_QRangeModel::removeColumns()
{
    QFETCH(Factory, factory);
    auto model = factory();
    QFETCH(const int, expectedColumnCount);
    QFETCH(const ChangeActions, changeActions);

    QCOMPARE(model->columnCount(), expectedColumnCount);
    QCOMPARE(model->removeColumn(0),
             changeActions.testFlag(ChangeAction::RemoveColumns));
}

void tst_QRangeModel::moveColumns()
{
    QFETCH(Factory, factory);
    auto model = factory();
    QFETCH(const int, expectedColumnCount);
    QFETCH(const ChangeActions, changeActions);

    QCOMPARE(model->columnCount(), expectedColumnCount);
    if (expectedColumnCount < 2)
        QSKIP("Cannot test moveColumns with a single-column model");

    const QVariant first = model->index(0, 0).data();
    const QVariant second = model->index(0, 1).data();
    const QVariant last = model->index(0, expectedColumnCount - 1).data();

    // various noops, should always fail
    QVERIFY(!model->moveColumns({}, 0, 1, {}, 0));
    QVERIFY(!model->moveColumns({}, 0, 1, {}, 1));

    QCOMPARE(model->moveColumns({}, 0, 1, {}, expectedColumnCount),
             bool(changeActions & ChangeAction::ChangeColumns));
    if (!(changeActions & ChangeAction::ChangeColumns))
        return;

    QCOMPARE(model->index(0, 0).data(), second);
    QCOMPARE(model->index(0, expectedColumnCount - 2).data(), last);
    QCOMPARE(model->index(0, expectedColumnCount - 1).data(), first);

    // the rest only makes sense for models with at least 3 columns
    if (expectedColumnCount >= 3) {
        // move all but one column to the end - this restores the order
        QVERIFY(model->moveColumns({}, 0, expectedColumnCount - 1,
                                {}, expectedColumnCount));
        QCOMPARE(model->index(0, 0).data(), first);
        QCOMPARE(model->index(0, 1).data(), second);
        QCOMPARE(model->index(0, expectedColumnCount - 1).data(), last);

        // move the last row step by step up to the top
        for (int column = model->columnCount() - 1; column > 0; --column)
            QVERIFY(model->moveColumn({}, column, {}, column - 1));
        QCOMPARE(model->index(0, 0).data(), last);
        // move all except the first row up - this restores the order again
        QVERIFY(model->moveColumns({}, 1, expectedColumnCount - 1, {}, 0));
        QCOMPARE(model->index(0, 0).data(), first);
        QCOMPARE(model->index(0, 1).data(), second);
        QCOMPARE(model->index(0, expectedColumnCount - 1).data(), last);
    }
}

void tst_QRangeModel::inconsistentColumnCount()
{
#ifndef QT_NO_DEBUG
    QTest::ignoreMessage(QtCriticalMsg, "QRangeModel: "
        "Column-range at row 1 is not large enough!");
#endif

    std::vector<std::vector<int>> fuzzyTable = {
        {0},
        {},
        {2},
    };
    QRangeModel model(fuzzyTable);
    QCOMPARE(model.columnCount(), 1);
    for (int row = 0; row < model.rowCount(); ++row) {
        auto debug = qScopeGuard([&]{
            qCritical() << "Test failed for row" << row << fuzzyTable.at(row).size();
        });
        const bool shouldWork = int(fuzzyTable.at(row).size()) >= model.columnCount();
        const auto index = model.index(row, model.columnCount() - 1);
        QCOMPARE(index.isValid(), shouldWork);
        // none of these should crash
        QCOMPARE(index.data().isValid(), shouldWork);
        QCOMPARE(model.setData(index, row + 5), shouldWork);
        QCOMPARE(model.clearItemData(index), shouldWork);
        debug.dismiss();
    }
}

void tst_QRangeModel::largeArrays()
{
    {
        std::array<int, 10000> largeArray = {};
        QRangeModel model(largeArray);
        const QModelIndex index = model.index(int(largeArray.size() - 1), 0);
        QCOMPARE(index.data(), 0);
    }

    {
        int largeArray[10000] = {};
        QRangeModel model(&largeArray);
        const QModelIndex index = model.index(int(std::size(largeArray)) - 1, 0);
        QCOMPARE(index.data(), 0);
    }

    {
        std::array<std::array<int, 10000>, 1> largeColumn = {};
        QRangeModel model(largeColumn);
        const QModelIndex index = model.index(int(largeColumn.size() - 1),
                                              int(largeColumn[0].size() - 1));
        QCOMPARE(index.data(), 0);
    }

    {
        using row = std::array<int, 10000>;
        std::array<std::shared_ptr<row>, 1> largeColumn = { std::make_shared<row>() };
        QRangeModel model(largeColumn);
        const QModelIndex index = model.index(int(largeColumn.size() - 1),
                                              int(largeColumn[0]->size() - 1));
        QCOMPARE(index.data(), 0);
    }
}

void tst_QRangeModel::mapsAsRange()
{
    {
        QRangeModel model(QMap<int, QString>{
            {1, u"eins"_s}
        });
        QCOMPARE(model.columnCount(), 1);
    }

    {
        QRangeModel model(QHash<int, QString>{
            {1, u"eins"_s}
        });
        QCOMPARE(model.columnCount(), 1);
    }

    {
        QRangeModel model(std::map<int, QString>{
            {1, u"eins"_s}
        });
        QCOMPARE(model.columnCount(), 2);
    }

    {
        QRangeModel model(std::unordered_map<int, QString>{
            {1, u"eins"_s}
        });
        QCOMPARE(model.columnCount(), 2);
    }
}

void tst_QRangeModel::spanAsRange()
{
    QList<int> list = {1, 2, 3};
    QSpan span(list);
    QRangeModel model(span);
}

void tst_QRangeModel::filterAsRange()
{
#if defined(__cpp_lib_ranges)
    auto view = std::views::iota(0, 100)
              | std::views::filter([](int i){ return 0 == i % 2; })
              | std::views::transform([](int i){ return i * i; });

    QRangeModel model(view);
    QCOMPARE(model.rowCount(), 50);
#else
    QSKIP("Test of std::ranges requires C++ 20");
#endif
}

template <>
struct QRangeModel::ItemAccess<QPolygon>
{
    static QVariant readRole(const QPolygon &polygon, int role)
    {
        if (role == Qt::DisplayRole) {
            QString string;
            bool first = true;
            for (const auto &point : polygon) {
                if (!first)
                    string += ";";
                else
                    first = false;
                string += u"%1/%2"_s.arg(point.x()).arg(point.y());
            }
            return string;
        }
        return QVariant();
    }

    static bool writeRole(QPolygon &target, const QVariant &value, int role)
    {
        target.clear();
        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            bool ok = false;
            for (auto entry : QStringTokenizer(value.toString(), u';')) {
                const auto tokens = entry.tokenize(u'/');
                auto coord = tokens.cbegin();
                QPoint point;
                point.rx() = coord->toInt(&ok);
                if (!ok)
                    break;
                ++coord;
                point.ry() = coord->toInt(&ok);
                if (!ok)
                    break;
                target += point;
            }
            return ok;
        }
        return false;
    }
};

void tst_QRangeModel::multiRoleContainer()
{
    static_assert(QRangeModelDetails::item_access<QPolygon>::hasReadRole);

    QList<QPolygon> listOfPolygons = {
        QPolygon{{0, 0}, {1, 0}, {1, 1}, {0, 1}},
        QPolygon{{0, 0}, {2, 0}, {2, 2}, {0, 2}},
        QPolygon{{0, 0}, {3, 0}, {3, 3}, {0, 3}},
    };

    QRangeModel model(std::ref(listOfPolygons));

    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(model.columnCount(), 1);

    const QModelIndex first = model.index(0, 0);
    QCOMPARE(first.data(), "0/0;1/0;1/1;0/1");
    QVERIFY(model.setData(first, "1/0;1/1;0/1;0/0"));
    QCOMPARE(first.data(), "1/0;1/1;0/1;0/0");
}

template <>
struct QRangeModel::RowOptions<QPoint>
{
    static constexpr auto rowCategory = QRangeModel::RowCategory::MultiRoleItem;
};

void tst_QRangeModel::multiRoleTuple()
{
    QList<QPoint> listOfPoints = {
        {0, 0},
        {1, 0},
        {1, 1},
        {0, 1},
    };

    QRangeModel model(listOfPoints);
    QCOMPARE(model.rowCount(), listOfPoints.size());
    QCOMPARE(model.columnCount(), 1);

    const QModelIndex item = model.index(1, 0);
    QCOMPARE(item.data().value<QPoint>(), listOfPoints.at(1));
    QVERIFY(model.setData(item, listOfPoints.back()));
    QCOMPARE(item.data().value<QPoint>(), listOfPoints.back());
}

namespace ADLTest
{
struct Value
{
    int x;

    template <typename V = Value>
    friend auto refTo(const V &)
    {
        static_assert(QtPrivate::type_dependent_false<V>(),
                      "refTo should never be found through ADL.");
    }
    template <typename V = Value>
    friend auto pointerTo(const V &)
    {
        static_assert(QtPrivate::type_dependent_false<V>(),
                      "pointerTo should never be found through ADL.");
    }
};

struct Range
{
    static inline bool beginCalled = false;
    static inline bool sizeCalled = false;

    friend Value *begin(Range &r)
    {
        Range::beginCalled = true;
        return r.values;
    }

    friend Value *end(Range &r)
    {
        // never called by QRM, only used in tree models
        return r.values + std::size(r.values);
    }

    friend size_t size(const Range &r)
    {
        Range::sizeCalled = true;
        return std::size(r.values);
    }

    Value values[3] = {{0}, {1}, {2}};
};
} // namespace ADLTest

void tst_QRangeModel::adlTest()
{
    QRangeModel adlModel(std::vector<ADLTest::Value>{});

    ADLTest::Range r;

    // compile tests
    {
        QRangeModel model(&r);
    }
    {
        QRangeModel model(std::make_unique<ADLTest::Range>());
    }
    {
        QRangeModel model(std::ref(r));
    }

    QRangeModel model(std::move(r));
    QCOMPARE(model.rowCount(), 3);
    const QModelIndex top = model.index(0, 0);
    const QModelIndex bottom = model.index(model.rowCount() - 1, 0);

    QVERIFY(top.isValid());
    QVERIFY(bottom.isValid());

    QVariant topData = model.data(top);
    QVariant bottomData = model.data(bottom);
    QCOMPARE(topData.value<ADLTest::Value>().x, top.row());
    QCOMPARE(bottomData.value<ADLTest::Value>().x, bottom.row());

    QVERIFY(ADLTest::Range::beginCalled);
    QVERIFY(ADLTest::Range::sizeCalled);
}

class ItemAccessItem : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString display MEMBER m_display)
    Q_PROPERTY(QColor decoration MEMBER m_decoration)
public:
    ItemAccessItem(const QString &display, QColor decoration)
        : m_display(display), m_decoration(decoration)
    {}

    QString m_display;
    QColor m_decoration;
};

template <>
struct QRangeModel::ItemAccess<ItemAccessItem>
{
    static QVariant readRole(const ItemAccessItem &item, int role)
    {
        switch (role) {
        case Qt::DisplayRole:
            return item.m_display.toUpper();
        case Qt::DecorationRole:
            return QColor(item.m_decoration.red(), item.m_decoration.green(),
                          item.m_decoration.blue(), 128);
        default:
            break;
        }
        return {};
    }

    static bool writeRole(ItemAccessItem &item, const QVariant &data, int role)
    {
        switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            item.m_display = data.toString();
            return true;
        default:
            break;
        }
        return false;
    }
};

void tst_QRangeModel::itemAccess_data()
{
    QTest::addColumn<std::shared_ptr<QRangeModel>>("model");

    QTest::addRow("raw")
        << std::make_shared<QRangeModel>(QList<ItemAccessItem *>{
            new ItemAccessItem{"one", Qt::red}
        });

    QTest::addRow("std::shared_ptr")
        << std::make_shared<QRangeModel>(QList<std::shared_ptr<ItemAccessItem>>{
            std::make_shared<ItemAccessItem>("one", Qt::red),
        });

    {
        auto data = std::vector<std::unique_ptr<ItemAccessItem>>{};
        data.emplace_back(std::make_unique<ItemAccessItem>("one", Qt::red));
        auto model = std::make_shared<QRangeModel>(std::move(data));
        QTest::addRow("std::unique_ptr")
            << model;
    }

    QTest::addRow("QSharedPtr")
        << std::make_shared<QRangeModel>(QList<QSharedPointer<ItemAccessItem>>{
            QSharedPointer<ItemAccessItem>(new ItemAccessItem{"one", Qt::red})
        });
}

void tst_QRangeModel::itemAccess()
{
    QFETCH(std::shared_ptr<QRangeModel>, model);

    QCOMPARE(model->columnCount(), 1);
    const QModelIndex index = model->index(0, 0);
    QCOMPARE(model->data(index), "ONE");
    QCOMPARE(model->data(index, Qt::DecorationRole).value<QColor>().alpha(), 128);
    QVERIFY(model->setData(index, "Two"));
    QCOMPARE(model->data(index), "TWO");
    QVERIFY(!model->setData(index, QVariant::fromValue(Qt::blue), Qt::DecorationRole));
}

void tst_QRangeModel::sortBasic()
{
    { // fast path: no PMIs
        QList<int> list = {1, 10, 3, 8, 5, 6, 7, 4, 9, 2, 11};
        auto sorted = list;
        std::sort(sorted.begin(), sorted.end(), [](int a, int b){ return a > b; });
        QRangeModel(&list).sort(0, Qt::DescendingOrder);
        QCOMPARE(list, sorted);
    }

    { // slow path: with PMIs
        QList<int> list = {1, 10, 3, 8, 5, 6, 7, 4, 9, 2, 11};
        auto sorted = list;
        std::sort(sorted.begin(), sorted.end());

        QRangeModel model(&list);
        QPersistentModelIndex pmi0(model.index(0, 0));
        QPersistentModelIndex pmi4(model.index(4, 0));
        const auto oldData0 = pmi0.data();
        const auto oldData4 = pmi4.data();

        model.sort(0, Qt::AscendingOrder);
        QCOMPARE(list, sorted);

        QCOMPARE(pmi0.data(), oldData0);
        QCOMPARE(pmi4.data(), oldData4);
    }

    { // two columns
        using Item = std::pair<QString, int>;
        QList<Item> list = {
            {"b", 2}, {"d", 4}, {"e", 5}, {"c", 3}, {"a", 1}
        };
        QRangeModel model(&list);
        QPersistentModelIndex pmi0(model.index(0, 0));
        QPersistentModelIndex pmi4(model.index(4, 1));
        const auto oldData0 = pmi0.data();
        const auto oldData4 = pmi4.data();

        model.sort(0, Qt::AscendingOrder);
        QCOMPARE(list, (QList<Item>{
            {"a", 1}, {"b", 2}, {"c", 3}, {"d", 4}, {"e", 5}
        }));
        QCOMPARE(pmi0.data(), oldData0);
        QCOMPARE(pmi4.data(), oldData4);

        model.sort(1, Qt::DescendingOrder);
        QCOMPARE(list, (QList<Item>{
            {"e", 5}, {"d", 4}, {"c", 3}, {"b", 2}, {"a", 1}
        }));
        QCOMPARE(pmi0.data(), oldData0);
        QCOMPARE(pmi4.data(), oldData4);
    }

    { // unordered
#ifndef QT_NO_DEBUG
        QTest::ignoreMessage(QtCriticalMsg, "QRangeModel: "
                             "Cannot compare items of type QPen in column 0!");
#endif
        QRangeModel model(QList<QPen>{{Qt::red}, {Qt::green}, {Qt::blue}});
        model.sort(0);
    }
}

void tst_QRangeModel::sortRole()
{
    QRangeModel model(QList<QMap<int, QVariant>>{
        {
            {Qt::DisplayRole, 4},
            {Qt::DecorationRole, 1},
        },
        {
            {Qt::DisplayRole, 3},
            {Qt::DecorationRole, 2},
        },
        {
            {Qt::DisplayRole, 2},
            {Qt::DecorationRole, 3},
        },
        {
            {Qt::DisplayRole, 1},
            {Qt::DecorationRole, 4},
        },
    });

    QModelIndex index = model.index(0, 0);
    model.setSortRole(Qt::DisplayRole);
    model.sort(0);
    QCOMPARE(model.data(index, Qt::DisplayRole), 1);
    QCOMPARE(model.data(index, Qt::DecorationRole), 4);

    index = model.index(0, 0);
    model.setSortRole(Qt::DecorationRole);
    model.sort(0);
    QCOMPARE(model.data(index, Qt::DisplayRole), 4);
    QCOMPARE(model.data(index, Qt::DecorationRole), 1);
}

void tst_QRangeModel::sortCollator_data()
{
    QTest::addColumn<QVariantList>("data");
    QTest::addColumn<QCollator>("collator");
    QTest::addColumn<QVariantList>("sorted");

    QVariantList caseData{"C", "b", "A", "d"};

    QCollator nullCollator = QCollator(QLocale::C);
    QCollator caseSensitive = nullCollator;
    caseSensitive.setCaseSensitivity(Qt::CaseSensitive);
    QCollator caseInsensitive;
    caseInsensitive.setCaseSensitivity(Qt::CaseInsensitive);
    QCollator localeAware = QCollator(QLocale::German);

    QTest::addRow("CaseSensitive")
        << caseData << caseSensitive << QVariantList{"A", "C", "b", "d"};
    if (caseInsensitive.compare("b", "C") < 0) {
        QTest::addRow("CaseInsensitive")
            << caseData << caseInsensitive << QVariantList{"A", "b", "C", "d"};
    } else {
        qInfo("Platform doesn't implement case insensitive collation.");
    }


    QVariantList i18nData{"a", "z", "ö", "ä"};

    QTest::addRow("LocaleUnaware")
        << i18nData << nullCollator << QVariantList{"a", "z", "ä", "ö"};
    if (localeAware.compare("ä", "z") < 0) {
        QTest::addRow("LocaleAware")
            << i18nData << localeAware << QVariantList{"a", "ä", "ö", "z"};
    } else {
        qInfo("Platform doesn't implement collation with a German locale.");
    }
}

void tst_QRangeModel::sortCollator()
{
    QFETCH(QVariantList, data);
    QFETCH(const QCollator, collator);
    QFETCH(const QVariantList, sorted);

    { // QVariant comparison
        QRangeModel model(std::ref(data));
        model.setSortCollator(collator);
        model.sort(0);
        QCOMPARE(data, sorted);
    }

    { // test shortcut for compile-time typed values
        QStringList stringData;
        for (const auto &var : std::as_const(data))
            stringData << var.toString();
        QStringList sortedStrings;
        for (const auto &var : std::as_const(sorted))
            sortedStrings << var.toString();
        QRangeModel model(std::ref(stringData));
        model.setSortCollator(collator);
        model.sort(0);
        QCOMPARE(stringData, sortedStrings);
    }
}

void tst_QRangeModel::sort()
{
    QFETCH(Factory, factory);
    QFETCH(ChangeActions, changeActions);
    auto model = factory();

    QTest::failOnWarning();

    model->sort(0, Qt::AscendingOrder);
    {
        QPersistentModelIndex pmi = model->index(0, 0);
        model->sort(0, Qt::DescendingOrder);
        if (changeActions.testFlag(ChangeAction::Sort))
            QCOMPARE_NE(pmi.row(), 0);
        else
            QCOMPARE(pmi.row(), 0);
    }

    const int column = model->columnCount() - 1;
    model->sort(column, Qt::DescendingOrder);
    {
        QPersistentModelIndex pmi = model->index(0, column);
        model->sort(column, Qt::AscendingOrder);
        if (changeActions.testFlag(ChangeAction::Sort))
            QCOMPARE_NE(pmi.row(), 0);
        else
            QCOMPARE(pmi.row(), 0);
    }
}

class Person : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString name MEMBER m_name)
    Q_PROPERTY(int age MEMBER m_age)
public:
    Person(const QString &name, int age)
        : m_name(name), m_age(age)
    {}
private:
    QString m_name;
    int m_age;
};

template <>
struct QRangeModel::RowOptions<Person>
{
    static constexpr auto rowCategory = QRangeModel::RowCategory::MultiRoleItem;
};

void tst_QRangeModel::matchBasic()
{
    {
        QList<int> list = {13, 23, 3, 34, 5};
        QRangeModel model(list);
        const QModelIndex start = model.index(0, 0);
        const auto result = model.match(start, Qt::DisplayRole, 3, -1, Qt::MatchExactly);
        QCOMPARE(result.size(), 1);
        QCOMPARE(model.data(result.at(0)), 3);
    }

    {
        QList<QString> list = {"1", "2", "35", "435", "5321"};
        QRangeModel model(list);
        const QModelIndex start = model.index(0, 0);
        const auto result = model.match(start, Qt::DisplayRole, "3", -1, Qt::MatchContains);
        QCOMPARE(result.size(), 3);
        QCOMPARE(model.data(result.at(0)), "35");
        QCOMPARE(model.data(result.at(1)), "435");
        QCOMPARE(model.data(result.at(2)), "5321");
    }

    {
        QList<QString> list = {"1", "2", "133", "43", "15"};
        QRangeModel model(list);
        const QModelIndex start = model.index(0, 0);
        const auto result = model.match(start, Qt::DisplayRole, "1", -1, Qt::MatchStartsWith);
        QCOMPARE(result.size(), 3);
        QCOMPARE(model.data(result.at(0)), "1");
        QCOMPARE(model.data(result.at(1)), "133");
        QCOMPARE(model.data(result.at(2)), "15");
    }

    {
        QList<QString> list = {"1", "2", "33", "43", "5"};
        QRangeModel model(list);
        const QModelIndex start = model.index(0, 0);
        const auto result = model.match(start, Qt::DisplayRole, "3", -1, Qt::MatchEndsWith);
        QCOMPARE(result.size(), 2);
        QCOMPARE(model.data(result.at(0)), "33");
        QCOMPARE(model.data(result.at(1)), "43");
    }

    {
        QList<QString> list = {"ALICE", "Alice", "AliCe", "ALIce"};
        QRangeModel model(list);
        const QModelIndex start = model.index(0, 0);
        const auto result = model.match(start, Qt::DisplayRole, "Alice", -1, Qt::MatchCaseSensitive);
        QCOMPARE(result.size(), 1);
        QCOMPARE(model.data(result.at(0)), "Alice");
    }

    {
        QList<int> list = {1, 2, 3, 2, 4, 2};
        QRangeModel model(list);
        const QModelIndex start = model.index(0, 0);
        const auto result = model.match(start, Qt::DisplayRole, 2, 1, Qt::MatchExactly);
        QCOMPARE(result.size(), 1);
        QCOMPARE(model.data(result.at(0)), 2);
    }

    {
        std::vector<std::unique_ptr<Person>> list;
        list.push_back(std::make_unique<Person>("Alice", 30));
        list.push_back(std::make_unique<Person>("Marie", 25));
        list.push_back(std::make_unique<Person>("Charlie", 30));

        QRangeModel model(std::move(list));
        const QModelIndex start = model.index(0, 0);

        const int nameRole = model.roleNames().key("name");
        const auto matchName = model.match(start, nameRole, "Marie", -1, Qt::MatchExactly);
        QCOMPARE(matchName.size(), 1);
        QCOMPARE(model.data(matchName.at(0), nameRole).toString(), "Marie");

        const int ageRole = model.roleNames().key("age");
        const auto matchAge = model.match(start, ageRole, 30, -1, Qt::MatchExactly);
        QCOMPARE(matchAge.size(), 2);
        QCOMPARE(model.data(matchAge.at(0), ageRole).toInt(), 30);
        QCOMPARE(model.data(matchAge.at(1), ageRole).toInt(), 30);
    }
}

void tst_QRangeModel::match()
{
    QFETCH(Factory, factory);
    QFETCH(QVariant, headerValue);
    auto model = factory();

    const int role = model->roleNames().key(headerValue.toByteArray());
    const QModelIndex start = model->index(0, 0);
    const QVariant value = model->data(start, role);
    { // find one
        const auto result = model->match(start, role, value, 1, Qt::MatchExactly);
        QCOMPARE(result.size(), 1);
        QCOMPARE(result.first(), start);
    }

    { // find all
        const QVariant value = model->data(start, role);
        const auto result = model->match(start, role, value, -1, Qt::MatchExactly);
        QVERIFY(result.size() >= 1);
        QCOMPARE(result.first(), start);
    }

    { // wrap
        const int lastRow = model->rowCount() - 1;
        const QModelIndex lastIndex = model->index(lastRow, 0);
        const auto result = model->match(lastIndex, role, value, 1,
                                         Qt::MatchExactly | Qt::MatchWrap);
        QVERIFY(result.size() >= 1);
    }
}

void tst_QRangeModel::matchCollator_data()
{
    using Opt = QCollator::CollationOption;

    QTest::addColumn<QVariantList>("data");
    QTest::addColumn<QCollator>("collator");
    QTest::addColumn<QString>("needle");
    QTest::addColumn<Qt::MatchFlags>("flags");
    QTest::addColumn<QVariantList>("expected");

    const QVariantList names = {u"Alice"_s, u"ALICE"_s, u"alice"_s};
    QCollator localeAware = QCollator(QLocale::English);
    localeAware.setOptions(Opt::CaseInsensitive);
    QCollator localeUnaware = QCollator(QLocale::C);

    QTest::newRow("NoCollationOptions")
        << QVariantList{u"foo"_s, u"bar"_s, u"foo"_s} << localeAware
        << u"foo"_s << Qt::MatchFlags(Qt::MatchFixedString) << QVariantList{u"foo"_s, u"foo"_s};

    // Qt::MatchCaseSensitive overides QCollator's CaseInsensitive
    // collation option
    QTest::newRow("MatchCaseSensitive")
            << names << localeAware << u"Alice"_s << (Qt::MatchFixedString | Qt::MatchCaseSensitive)
            << QVariantList{u"Alice"_s};

    // Default case insensitive setting overrides QCollator's
    // CaseSensitive collation option
    localeAware.setCaseSensitivity(Qt::CaseSensitive);
    QCollator probe(QLocale::English);
    probe.setCaseSensitivity(Qt::CaseInsensitive);
    if (probe.compare(u"a", u"A") == 0) {
        QTest::newRow("DefaultOverride")
                << names << localeAware << u"alice"_s
                << Qt::MatchFlags(Qt::MatchFixedString) << names;
    } else {
        qInfo("Case-insensitive collation unsupported on this platform.");
    }

    localeAware.setOptions(Opt::CaseInsensitive | Opt::DiacriticInsensitive);
    if (localeAware.compare(u"resume", u"résumé") == 0) {
        QTest::newRow("DiacriticInsensitive")
            << QVariantList{u"résumé"_s, u"RÉSUMÉ"_s, u"other"_s} << localeAware
            << u"resume"_s << Qt::MatchFlags(Qt::MatchFixedString)
            << QVariantList{u"résumé"_s, u"RÉSUMÉ"_s};
        QTest::newRow("DiacriticInsensitiveStarts")
            << QVariantList{u"résumé"_s, u"RÉSUMÉ"_s, u"other"_s} << localeAware
            << u"re"_s << Qt::MatchFlags(Qt::MatchStartsWith)
            << QVariantList{u"résumé"_s, u"RÉSUMÉ"_s};
        QTest::newRow("DiacriticInsensitiveEnds")
            << QVariantList{u"résumé"_s, u"RÉSUMÉ"_s, u"other"_s} << localeAware
            << u"me"_s << Qt::MatchFlags(Qt::MatchEndsWith)
            << QVariantList{u"résumé"_s, u"RÉSUMÉ"_s};
    } else {
        qInfo("Ignoring diacritic marks unsupported on this platform.");
    }

    QTest::newRow("DiacriticInsensitiveContains")
            << QVariantList{u"résumé"_s, u"xyz"_s} << localeAware
            << u"resume"_s << Qt::MatchFlags(Qt::MatchContains)
            << QVariantList{u"résumé"_s};

    QTest::newRow("DiacriticInsensitiveContainsSubString")
            << QVariantList{u"résumé"_s, u"xyz"_s} << localeAware
            << u"esum"_s << Qt::MatchFlags(Qt::MatchContains)
            << QVariantList{u"résumé"_s};

    // Locale unaware - collation options unavailable
    localeUnaware.setOptions(Opt::DiacriticInsensitive);
    QTest::newRow("LocaleUnaware")
        << QVariantList{u"résumé"_s, u"resume"_s, u"other"_s} << localeUnaware
        << u"resume"_s << Qt::MatchFlags(Qt::MatchFixedString)
        << QVariantList{u"resume"_s};

    localeAware.setOptions(Opt::IgnorePunctuation);
    if (localeAware.compare(u"abc", u"a.b.c") == 0) {
        QTest::newRow("IgnorePunctuation")
            << QVariantList{u"a.b.c"_s, u"abc"_s, u"xyz"_s} << localeAware
            << u"abc"_s << Qt::MatchFlags(Qt::MatchFixedString)
            << QVariantList{u"a.b.c"_s, u"abc"_s};
    } else {
        qInfo("Ignoring punctuation unsupported on this platform.");
    }

    // Boundary, using plain ASCII with exact matches so that they produce
    // the same result regardless of which collation options a platform supports.
    QCollator plain = QCollator(QLocale::C);
    const QVariantList haystack = {u"abcdef"_s, u"xyz"_s};

    QTest::newRow("ContainsAtStart")
        << haystack << plain << u"abc"_s << Qt::MatchFlags(Qt::MatchContains)
        << QVariantList{u"abcdef"_s};

    QTest::newRow("ContainsAtEnd")
        << haystack << plain << u"def"_s << Qt::MatchFlags(Qt::MatchContains)
        << QVariantList{u"abcdef"_s};

    QTest::newRow("ContainsExactLength")
        << haystack << plain << u"abcdef"_s << Qt::MatchFlags(Qt::MatchContains)
        << QVariantList{u"abcdef"_s};

    QTest::newRow("ContainsNeedleTooLong")
        << haystack << plain << u"abcdefghij"_s << Qt::MatchFlags(Qt::MatchContains)
        << QVariantList{};
    QTest::newRow("StartsWithNeedleTooLong")
        << haystack << plain << u"abcdefghij"_s << Qt::MatchFlags(Qt::MatchStartsWith)
        << QVariantList{};
    QTest::newRow("EndsWithNeedleTooLong")
        << haystack << plain << u"abcdefghij"_s << Qt::MatchFlags(Qt::MatchEndsWith)
        << QVariantList{};
    QTest::newRow("ContainsNeedleLongerThanSomeItems")
        << haystack << plain << u"xyzw"_s << Qt::MatchFlags(Qt::MatchContains)
        << QVariantList{};
}

void tst_QRangeModel::matchCollator()
{
    QFETCH(QVariantList, data);
    QFETCH(const QCollator, collator);
    QFETCH(const QString, needle);
    QFETCH(const Qt::MatchFlags, flags);
    QFETCH(const QVariantList, expected);

    { // QVariant comparison
        QRangeModel model(std::ref(data));
        model.setMatchCollator(collator);

        QVariantList matched;
        for (const QModelIndex &index : model.match(model.index(0, 0), Qt::DisplayRole,
                                                    needle, -1, flags)) {
            matched << model.data(index);
        }
        QCOMPARE(matched, expected);
    }

    { // test shortcut for compile-time typed values
        QStringList stringData;
        for (const QVariant &value : std::as_const(data))
            stringData << value.toString();
        QRangeModel model(std::ref(stringData));
        model.setMatchCollator(collator);

        QVariantList matched;
        for (const QModelIndex &index : model.match(model.index(0, 0), Qt::DisplayRole,
                                                    needle, -1, flags)) {
            matched << model.data(index);
        }
        QCOMPARE(matched, expected);
    }
}

QTEST_MAIN(tst_QRangeModel)
#include "tst_qrangemodel.moc"
