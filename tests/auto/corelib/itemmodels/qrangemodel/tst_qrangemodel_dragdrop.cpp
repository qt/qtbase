// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tst_qrangemodel.h"
#include <QtCore/qmimedata.h>
#include <QtCore/qstringlistmodel.h>
#include <QtCore/qxmlstream.h>

using namespace Qt::StringLiterals;

void tst_QRangeModel::dragDropActions_data()
{
    QTest::addColumn<Factory>("factory");
    QTest::addColumn<Qt::DropActions>("defaultDragActions");
    QTest::addColumn<Qt::DropActions>("possibleDragActions");
    QTest::addColumn<Qt::DropActions>("defaultDropActions");
    QTest::addColumn<Qt::DropActions>("possibleDropActions");

    m_data.reset(new Data);

    QTest::addRow("Fixed-sized") << Factory([this]() -> std::unique_ptr<QAbstractItemModel> {
        return std::make_unique<QRangeModel>(std::ref(m_data->fixedArrayOfNumbers));
    }) << Qt::DropActions(Qt::CopyAction) << Qt::DropActions(Qt::CopyAction|Qt::MoveAction|Qt::LinkAction)
       << Qt::DropActions(Qt::CopyAction) << Qt::DropActions(Qt::CopyAction|Qt::MoveAction|Qt::LinkAction);
    QTest::addRow("Fixed-Columns") << Factory([this]() -> std::unique_ptr<QAbstractItemModel> {
        return std::make_unique<QRangeModel>(std::ref(m_data->vectorOfFixedColumns));
    }) << Qt::DropActions(Qt::CopyAction) << Qt::DropActions(Qt::CopyAction|Qt::MoveAction|Qt::LinkAction)
       << Qt::DropActions(Qt::CopyAction) << Qt::DropActions(Qt::CopyAction|Qt::MoveAction|Qt::LinkAction);
    QTest::addRow("Read-Only") << Factory([this]() -> std::unique_ptr<QAbstractItemModel> {
        return std::make_unique<QRangeModel>(std::ref(m_data->constTableOfNumbers));
    }) << Qt::DropActions(Qt::CopyAction) << Qt::DropActions(Qt::CopyAction|Qt::LinkAction)
       << Qt::DropActions(Qt::IgnoreAction) << Qt::DropActions(Qt::IgnoreAction);
}

void tst_QRangeModel::dragDropActions()
{
    QFETCH(Factory, factory);
    QFETCH(Qt::DropActions, defaultDragActions);
    QFETCH(Qt::DropActions, possibleDragActions);
    QFETCH(Qt::DropActions, defaultDropActions);
    QFETCH(Qt::DropActions, possibleDropActions);
    auto model = factory();
    auto *rangeModel = qobject_cast<QRangeModel *>(model.get());

    QCOMPARE(model->supportedDragActions(), defaultDragActions);
    QCOMPARE(model->supportedDropActions(), defaultDropActions);

    rangeModel->setSupportedDragActions(Qt::CopyAction | Qt::MoveAction | Qt::LinkAction);
    QCOMPARE(model->supportedDragActions(), possibleDragActions);
    rangeModel->setSupportedDropActions(Qt::CopyAction | Qt::MoveAction | Qt::LinkAction);
    QCOMPARE(model->supportedDropActions(), possibleDropActions);
}

using DragDropRow = std::tuple<QString, int>;

template <>
struct QRangeModel::RowOptions<DragDropRow>
{
    static QStringList mimeTypes() { return {u"application/vnd.text.list"_s}; }
    template <typename Range>
    static QMimeData *mimeData(Range &&rows)
    {
        QMimeData *result = new QMimeData;
        QByteArray data;
        QTextStream stream(&data, QIODevice::WriteOnly);
        for (const auto &[row, index] : rows)
            stream << std::get<0>(row) << "," << std::get<1>(row) << Qt::endl;
        result->setData(mimeTypes().first(), data);
        return result;
    }

    template <typename Range>
    static bool dropMimeData(const QMimeData *data, Range &&inserter)
    {
        QByteArray textData = data->data(mimeTypes().first());
        if (textData.isEmpty())
            return false;
        QTextStream stream(&textData, QIODevice::ReadOnly);
        while (!stream.atEnd()) {
            const auto line = stream.readLine().split(",");
            inserter = DragDropRow{line[0], line[1].toInt()};
        }
        return true;
    }
};

static_assert(QRangeModelDetails::hasMimeDataIndexList<DragDropRow>);

struct DragDropItem
{
    Q_GADGET
    Q_PROPERTY(QString string MEMBER m_string)
    Q_PROPERTY(int number MEMBER m_number)

public:
    DragDropItem() = default;
    DragDropItem(const QString &string, int number)
        : m_string(string), m_number(number)
    {}

    QString m_string;
    int m_number = 0;
};

template <>
struct QRangeModel::ItemAccess<DragDropItem>
{
    static QVariant readRole(const DragDropItem &item, int role)
    {
        switch (role) {
        case Qt::DisplayRole:
            return item.m_string;
        case Qt::DecorationRole:
            return item.m_number;
        }
        return {};
    }
    static bool writeRole(DragDropItem &item, const QVariant &value, int role)
    {
        switch (role) {
        case Qt::DisplayRole:
            item.m_string = value.toString();
            break;
        case Qt::DecorationRole:
            item.m_number = value.toInt();
            break;
        default:
            return false;
        }
        return true;
    }

    static QStringList mimeTypes() { return {u"text/html"_s}; }
    template <typename Items>
    static QMimeData *mimeData(const Items &items)
    {
        if (items.isEmpty())
            return nullptr;
        QByteArray data;
        QXmlStreamWriter stream(&data);
        stream.writeStartElement("ul");
        for (const auto &[item, index] : items) {
            stream.writeTextElement("li", item.m_string);
        }
        stream.writeEndElement();

        QMimeData *mimeData = new QMimeData;
        mimeData->setData(mimeTypes().first(), data);
        return mimeData;
    }
};

static_assert(QRangeModelDetails::item_access<DragDropItem>::hasMimeTypes);

struct NoDragDropRow : std::tuple<int> {};

template <>
struct QRangeModel::RowOptions<NoDragDropRow>
{
    static QStringList mimeTypes() { return {}; }
};

void tst_QRangeModel::mimeTypes_data()
{
    QTest::addColumn<Factory>("factory");
    QTest::addColumn<QStringList>("expected");

    const QStringList defaultMimeTypes = QStringListModel().mimeTypes();

    QTest::addRow("QStringList") << Factory([]() -> std::unique_ptr<QAbstractItemModel> {
        return std::make_unique<QRangeModel>(QStringList{});
    }) << defaultMimeTypes;

    QTest::addRow("QList<DragDropRow>") << Factory([]() -> std::unique_ptr<QAbstractItemModel> {
        return std::make_unique<QRangeModel>(QList<DragDropRow>{});
    }) << QRangeModel::RowOptions<DragDropRow>::mimeTypes();

    QTest::addRow("QList<DragDropItem>") << Factory([]() -> std::unique_ptr<QAbstractItemModel> {
        return std::make_unique<QRangeModel>(QList<DragDropItem>{
            {"one", 1},
            {"two", 2},
            {"three", 3},
        });
    }) << QRangeModel::ItemAccess<DragDropItem>::mimeTypes();

    QTest::addRow("QList<NoDragDropRow>") << Factory([]() -> std::unique_ptr<QAbstractItemModel> {
        return std::make_unique<QRangeModel>(QList<NoDragDropRow>{});
    }) << QRangeModel::RowOptions<NoDragDropRow>::mimeTypes();
}

void tst_QRangeModel::mimeTypes()
{
    QFETCH(Factory, factory);
    QFETCH(const QStringList, expected);

    auto model = factory();

    const QStringList actualMimeTypes = model->mimeTypes();
    QCOMPARE(actualMimeTypes, expected);
}

using MimeDataList = QList<std::pair<QString, QByteArray>>;

void tst_QRangeModel::mimeData_data()
{
    QTest::addColumn<Factory>("factory");
    QTest::addColumn<QList<QPoint>>("cells");
    QTest::addColumn<MimeDataList>("expected");

    QTest::addRow("QList<DragDropRow>") << Factory([]() -> std::unique_ptr<QAbstractItemModel> {
        return std::make_unique<QRangeModel>(QList<DragDropRow>{
            DragDropRow{u"one"_s, 1},
            DragDropRow{u"two"_s, 2},
            DragDropRow{u"three"_s, 3}
        });
    }) << QList<QPoint>{{0, 0}, {1, 0}}
       << MimeDataList{std::pair{u"application/vnd.text.list"_s, QByteArray("one,1\n")}};

    QTest::addRow("QList<DragDropRow *>/valid-rows") << Factory([]() -> std::unique_ptr<QAbstractItemModel> {
        QList<DragDropRow *> rows{
            new DragDropRow{u"one"_s, 1},
            nullptr,
            new DragDropRow{u"three"_s, 3}
        };
        return std::make_unique<QRangeModel>(std::move(rows));
    }) << QList<QPoint>{{0, 0}, {1, 0}, {0, 2}, {1, 2}}
       << MimeDataList{std::pair{u"application/vnd.text.list"_s, QByteArray("one,1\nthree,3\n")}};

    QTest::addRow("QList<DragDropRow *>/null-row") << Factory([]() -> std::unique_ptr<QAbstractItemModel> {
        QList<DragDropRow *> rows{
            new DragDropRow{u"one"_s, 1},
            nullptr,
            new DragDropRow{u"three"_s, 3}
        };
        return std::make_unique<QRangeModel>(std::move(rows));
    }) << QList<QPoint>{{0, 1}, {1, 1}}
       << MimeDataList{std::pair{u"application/vnd.text.list"_s, QByteArray(",0\n")}};

    QTest::addRow("QList<DragDropItem>") << Factory([]() -> std::unique_ptr<QAbstractItemModel> {
        return std::make_unique<QRangeModel>(QList<DragDropItem>{
            {"one", 1},
            {"two", 2},
            {"three", 3},
        });
    }) << QList<QPoint>{{0, 0}, {0, 2}}
       << MimeDataList{std::pair{u"text/html"_s,
                                 QByteArray("<ul><li>one</li><li>three</li></ul>")}};

    QTest::addRow("QList<DragDropItem *>/valid-items") << Factory([]() -> std::unique_ptr<QAbstractItemModel> {
        QList<DragDropItem*> items{
            new DragDropItem{u"one"_s, 1},
            nullptr,
            new DragDropItem{u"three"_s, 3}
        };
        return std::make_unique<QRangeModel>(std::move(items));
    }) << QList<QPoint>{{0, 0}, {0, 2}}
       << MimeDataList{std::pair{u"text/html"_s,
                                 QByteArray("<ul><li>one</li><li>three</li></ul>")}};

    QTest::addRow("QList<DragDropItem *>/null-item") << Factory([]() -> std::unique_ptr<QAbstractItemModel> {
        QList<DragDropItem*> items{
            new DragDropItem{u"one"_s, 1},
            nullptr,
            new DragDropItem{u"three"_s, 3}
        };
        return std::make_unique<QRangeModel>(std::move(items));
    }) << QList<QPoint>{{0, 1}}
       << MimeDataList{std::pair{u"text/html"_s, QByteArray("<ul><li></li></ul>")}};
}


void tst_QRangeModel::mimeData()
{
    QFETCH(Factory, factory);
    QFETCH(const QList<QPoint>, cells);
    QFETCH(const MimeDataList, expected);

    auto model = factory();

    QModelIndexList indexes;
    for (const auto &cell : cells) {
        auto index = model->index(cell.y(), cell.x());
        QVERIFY(index.isValid());
        indexes.append(index);
    }

    QMimeData *data = model->mimeData(indexes);
    QVERIFY(data || expected.isEmpty());

    MimeDataList actual;
    for (const auto &format : data->formats())
        actual.append({format, data->data(format)});

    QCOMPARE(actual, expected);
}

void tst_QRangeModel::dropMimeData_data()
{
    QTest::addColumn<Factory>("factory");
    QTest::addColumn<MimeDataList>("mimeData");
    QTest::addColumn<int>("expectedRowCount");

    QTest::addRow("QList<DragDropRow>") << Factory([]() -> std::unique_ptr<QAbstractItemModel> {
        return std::make_unique<QRangeModel>(QList<DragDropRow>{});
    }) << MimeDataList{std::pair{u"application/vnd.text.list"_s, QByteArray("one,1\n")}}
       << 1;

    // Dropping into a pointer-row model allocates the new row and sets its data
    QTest::addRow("QList<DragDropRow*>") << Factory([]() -> std::unique_ptr<QAbstractItemModel> {
        return std::make_unique<QRangeModel>(QList<DragDropRow*>{});
    }) << MimeDataList{std::pair{u"application/vnd.text.list"_s, QByteArray("one,1\n")}}
       << 1;
}

void tst_QRangeModel::dropMimeData()
{
    QFETCH(Factory, factory);
    QFETCH(MimeDataList, mimeData);
    QFETCH(int, expectedRowCount);

    auto model = factory();

    auto mime = std::make_unique<QMimeData>();
    for (const auto &data : mimeData)
        mime->setData(data.first, data.second);

    QVERIFY(model->dropMimeData(mime.get(), Qt::CopyAction, -1, -1, {}));
    QCOMPARE(model->rowCount(), expectedRowCount);
}

// Flags for null pointer rows and items
void tst_QRangeModel::dragDropFlagsNullPointer()
{
    // Rows as raw pointers: null rows must not be flagged as draggable
    {
        QList<DragDropRow *> rows{new DragDropRow{u"one"_s, 1}, nullptr};
        QRangeModel model(std::move(rows));
        const QModelIndex validRow = model.index(0, 0);
        const QModelIndex nullRow = model.index(1, 0);
        QVERIFY(validRow.flags().testFlag(Qt::ItemIsDragEnabled));
        QVERIFY(!nullRow.flags().testFlag(Qt::ItemIsDragEnabled));
        QVERIFY(nullRow.flags().testFlag(Qt::ItemIsDropEnabled));
    }

    // Items as raw pointers: null items must not be flagged as draggable
    {
        QList<DragDropItem *> items{new DragDropItem{u"one"_s, 1}, nullptr};
        QRangeModel model(std::move(items));
        const QModelIndex validItem = model.index(0, 0);
        const QModelIndex nullItem = model.index(1, 0);
        QVERIFY(validItem.flags().testFlag(Qt::ItemIsDragEnabled));
        QVERIFY(!nullItem.flags().testFlag(Qt::ItemIsDragEnabled));
        QVERIFY(nullItem.flags().testFlag(Qt::ItemIsDropEnabled));
    }
}

// Dropping onto a model with pre-existing null pointer rows:
// Uses a fixed-size array (can't insert rows) so the drop overwrites
// existing entries. The model allocates a new row object to replace
// each null pointer that data is dropped onto.
void tst_QRangeModel::dropMimeDataNullRow()
{
    std::array<DragDropRow*, 3> rows = {};  // all null pointers
    {
        QRangeModel model(std::ref(rows));
        QCOMPARE(model.rowCount(), 3);

        // All rows are null pointers: not draggable, data reads return invalid
        for (int r = 0; r < 3; ++r) {
            QVERIFY(!model.index(r, 0).flags().testFlag(Qt::ItemIsDragEnabled));
            QVERIFY(!model.data(model.index(r, 0), Qt::DisplayRole).isValid());
        }

        // Drop "one,1" at row 0. The fixed-size array cannot grow so the model
        // overwrites existing rows. Row 0 is a null pointer, so the model
        // allocates a new DragDropRow before writing the dropped data into it.
        auto mime = std::make_unique<QMimeData>();
        mime->setData(u"application/vnd.text.list"_s, QByteArray("one,1\n"));
        QVERIFY(model.dropMimeData(mime.get(), Qt::CopyAction, 0, 0, {}));

        // Row 0 is now populated with the dropped data
        QCOMPARE(model.data(model.index(0, 0), Qt::DisplayRole).toString(), u"one");
        QCOMPARE(model.data(model.index(0, 1), Qt::DisplayRole).toInt(), 1);

        // Rows 1 and 2 were not targeted by the drop and remain null
        QVERIFY(!model.data(model.index(1, 0), Qt::DisplayRole).isValid());
        QVERIFY(!model.data(model.index(2, 0), Qt::DisplayRole).isValid());
    }
    // The model used std::ref and did not take ownership, so clean up manually.
    // Rows 1 and 2 are still null; deleting null is safe.
    delete rows[0];
    rows[0] = nullptr;
}

#include "tst_qrangemodel_dragdrop.moc"
