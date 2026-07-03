// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore>
#include <QtWidgets>

#if __has_include(<QtQml>)
#define QUICK_UI
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>
#include <QQuickWidget>
#include <QQuickItem>
#elif defined(Q_CC_MSVC)
#pragma message "Building without Quick UI"
#else
#warning "Building without Quick UI"
#endif

#include <list>
#include <ranges>
#include <vector>

using namespace Qt::StringLiterals;

class Gadget
{
    Q_GADGET
    Q_PROPERTY(QString display READ display WRITE setDisplay)
    Q_PROPERTY(QColor decoration READ decoration WRITE setDecoration)
    Q_PROPERTY(QString toolTip READ toolTip WRITE setToolTip)
    Q_PROPERTY(QString user READ display WRITE setDisplay)
public:
    Gadget() = default;

    Gadget(const QString &display, QColor decoration, const QString &toolTip)
        : m_display(display), m_decoration(decoration), m_toolTip(toolTip)
    {
    }

    QString display() const { return m_display; }
    void setDisplay(const QString &display) { m_display = display; }
    QColor decoration() const { return m_decoration; }
    void setDecoration(QColor decoration) { m_decoration = decoration; }
    QString toolTip() const { return m_toolTip.isEmpty() ? m_display : m_toolTip; }
    void setToolTip(const QString &toolTip) { m_toolTip = toolTip; }

private:
    friend QDebug operator<<(QDebug dbg, const Gadget &gadget)
    {
        dbg << "Gadget(" << gadget.m_display << gadget.m_decoration << gadget.m_toolTip << ")";
        return dbg;
    }
    QString m_display;
    QColor m_decoration;
    QString m_toolTip;
};

template <>
struct QRangeModel::RowOptions<Gadget>
{
    static Qt::ItemFlags flags(const Gadget &)
    {
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemNeverHasChildren
             | Qt::ItemIsEditable
             | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
    }

    static QStringList mimeTypes()
    {
        return QStringList{u"text/plain"_s};
    }

    template <typename Range>
    static QMimeData *mimeData(const Range &gadgets)
    {
        QByteArray data;
        QTextStream stream(&data, QIODevice::WriteOnly);
        for (const auto &row : gadgets) {
            if (!row.isValid()) {
                qCritical("Skipping invalid row");
                continue;
            }
            const auto &[gadget, index] = row;
            stream << Qt::flush;
            if (index.column() < 0) {
                stream << gadget.display() << ',' << gadget.decoration().name() << ','
                       << gadget.toolTip() << Qt::endl;
            } else {
                switch (index.column()) {
                case 0:
                    stream << gadget.display() << ",,";
                    break;
                case 1:
                    stream << ',' << gadget.decoration().name() << u',';
                    break;
                case 2:
                    stream << ",," << gadget.toolTip();
                    break;
                }
                stream << Qt::endl;
            }
        }
        if (data.isEmpty())
            return nullptr;
        QMimeData *mimeData = new QMimeData;
        mimeData->setData(mimeTypes().first(), data);
        return mimeData;
    }

    static QRangeModel::DropOperation dropMimeData(const QMimeData *mimeData, auto &&inserter)
    {
        if (!mimeData->hasFormat(mimeTypes().first()))
            return QRangeModel::DropOperation::DontDrop;
        QByteArray data = mimeData->data(mimeTypes().first());
        if (data.isEmpty())
            return QRangeModel::DropOperation::DontDrop;
        QTextStream stream(&data, QIODevice::ReadOnly);
        int newRow = 0;
        while (!stream.atEnd()) {
            const auto line = stream.readLine().split(",");
            if (line.isEmpty())
                continue;
            inserter = {Gadget(line.size() > 0 ? line[0] : QString(),
                              QColor::fromString(line.size() > 1 ? line[1] : QString()),
                              line.size() > 2 ? line[2] : QString()),
                        newRow};
            ++newRow; // += 2 would leave a gap
        }
        return QRangeModel::DropOperation::Automatic;
    }

    // simplified version, only called when the default implementation
    // accepts the drop, so action and mime type check already done
    static bool canDropMimeData(const QMimeData *)
    {
        return true;
    }

    // a full implementation to only support drops in between items
    // static bool canDropMimeData(const QMimeData *data, Qt::DropAction, int row, int column, const QModelIndex &)
    // {
    //     if (!data->hasFormat(mimeTypes().first()))
    //         return false;
    //     return row > -1 && column > -1;
    // }

    static QVariant headerData(int section, int role)
    {
        if (role == Qt::DisplayRole) {
            switch (section) {
            case 0: return QCoreApplication::translate("Gadget", "Display");
            case 1: return QCoreApplication::translate("Gadget", "Decoration");
            case 2: return QCoreApplication::translate("Gadget", "ToolTip");
            default:
                break;
            }
        }
        return {};
    }
};

template <>
struct QRangeModel::ItemAccess<Gadget>
{
    using RowOptions = typename QRangeModel::RowOptions<Gadget>;
    static QMimeData *mimeData(auto &&gadgets)
    {
        if (gadgets.isEmpty())
            return nullptr;

        const QModelIndex topLeft = gadgets.first().index();
        const QModelIndex bottomRight = gadgets.last().index();
        QRect span = {QPoint(topLeft.column(), topLeft.row()),
                      QPoint(bottomRight.column(), bottomRight.row())};
        span.moveTo(0, 0);

        QByteArray data;
        QTextStream stream(&data, QIODevice::WriteOnly);

        QModelIndex lastIndex;
        for (const auto &[gadget, index] : gadgets) {
            stream << Qt::flush;
            if (index.row() != lastIndex.row() && stream.pos()) {
                stream.seek(stream.pos() - 1);
                stream << Qt::endl;
            }
            stream << gadget.display() << ','
                   << gadget.decoration().name()
                   << ',' << gadget.toolTip()
                   << ';';
            lastIndex = index;
        }
        stream << Qt::endl;
        if (data.isEmpty())
            return nullptr;
        QMimeData *mimeData = new QMimeData;
        mimeData->setData(RowOptions::mimeTypes().first(), data);
        return mimeData;
    }

    static bool dropMimeData(const QMimeData *mimeData, auto inserter)
    {
        if (!mimeData->hasFormat(RowOptions::mimeTypes().first()))
            return false;
        QByteArray data = mimeData->data(RowOptions::mimeTypes().first());
        if (data.isEmpty())
            return false;

        QTextStream stream(&data, QIODevice::ReadOnly);
        while (!stream.atEnd()) {
            const QString row = stream.readLine();
            if (row.isEmpty())
                continue;
            const QStringList cells = row.split(u';', Qt::SkipEmptyParts);
            for (const auto &cell : cells) {
                const QStringList values = cell.split(u',');
                inserter = Gadget(
                    values.size() > 0 ? values[0] : QString(),
                    QColor::fromString(values.size() > 1 ? values[1] : QString()),
                    values.size() > 2 ? values[2] : QString()
                );
            }
        }
        return true;
    }
};

static_assert(QRangeModelDetails::hasHeaderData<Gadget>);
static_assert(QRangeModelDetails::hasRowFlags<Gadget>);
static_assert(QRangeModelDetails::hasMimeDataRowSpan<Gadget>);
static_assert(QRangeModelDetails::hasDropMimeData<Gadget>);
static_assert(QRangeModelDetails::item_access<Gadget>::hasMimeData);
static_assert(QRangeModelDetails::item_access<Gadget>::hasDropMimeData);

struct QMetaEnumerator
{
    struct iterator
    {
        using difference_type = int;
        using size_type = int;
        using pointer = void;
        using iterator_category = std::input_iterator_tag;
        using value_type = std::tuple<int, QByteArray, int>;
        using reference = value_type;
        using const_reference = const value_type;

        friend constexpr iterator &operator++(iterator &that)
        { ++that.m_index; return that; }
        friend constexpr iterator operator++(iterator &that, int)
        { auto copy = that; ++that.m_index; return copy; }
        friend constexpr iterator &operator+=(iterator &that, int n)
        { that.m_index += n; return that; }

        friend constexpr bool comparesEqual(const iterator &lhs, const iterator &rhs) noexcept
        {
            return lhs.m_index == rhs.m_index && lhs.m_enum == rhs.m_enum;
        }

        friend constexpr Qt::strong_ordering compareThreeWay(const iterator &lhs,
                                                             const iterator &rhs) noexcept
        {
            return Qt::compareThreeWay(lhs.m_index, rhs.m_index);
        }
        Q_DECLARE_STRONGLY_ORDERED(iterator)

        const_reference operator*() const
        { return {m_index, m_enum->key(m_index), m_enum->value(m_index)}; }

        const QMetaEnum *m_enum = nullptr;
        int m_index = 0;
    };

    static_assert(std::input_iterator<iterator>);

    using size_type = iterator::size_type;
    using value_type = iterator::value_type;
    using const_iterator = iterator;

    template <typename Enum>
    explicit QMetaEnumerator(Enum) noexcept
        : m_enum(QMetaEnum::fromType<Enum>())
    {}

    const_iterator begin() const { return iterator{&m_enum, 0}; }
    const_iterator end() const { return iterator{&m_enum, size()}; }
    size_type size() const { return m_enum.keyCount(); }

private:
    const QMetaEnum m_enum;
};

struct TreeRow;
using Tree = std::vector<TreeRow>;

struct TreeRow
{
public:
    TreeRow() = default;

    TreeRow(const QString &name, const QString &title)
        : m_name(name), m_title(title)
    {}

    ~TreeRow() = default;
    TreeRow(TreeRow &&other) = default;
    TreeRow &operator=(TreeRow &&other) = default;

    template <typename ...Args>
    TreeRow &addChild(Args&& ...args)
    {
        TreeRow *oldData = nullptr;
        if (!m_children) {
            m_children.emplace(Tree{});
            m_children->reserve(10);
        } else {
            oldData = m_children->data();
        }
        TreeRow &res = m_children->emplace_back(args...);
        // this is why trees of values in an iterator-invalidating container
        // are a bad idea.
        if (oldData && oldData != m_children->data())
            qWarning() << "Reallocated!";
        res.m_parent = this;
        return res;
    }

    static Tree fromJsonArray(const QJsonArray &array)
    {
        Tree tree;
        for (const auto &value : array) {
            if (!value.isObject())
                continue;
            QJsonObject object = value.toObject();
            TreeRow treeRow{
                object["name"].toString(),
                object["title"].toString()
            };
            if (QJsonValue maybeChildren = object["children"]; maybeChildren.isArray())
                treeRow.m_children = fromJsonArray(maybeChildren.toArray());
            tree.emplace_back(std::move(treeRow));
        }
        return tree;
    }

    // tree traversal protocol implementation
    TreeRow *parentRow() const { return m_parent; }
    void setParentRow(TreeRow *parent) { m_parent = parent; }
    const std::optional<Tree> &childRows() const { return m_children; }
    std::optional<Tree> &childRows() { return m_children; }

private:
    QString m_name;
    QString m_title;

    TreeRow *m_parent = nullptr;
    std::optional<Tree> m_children = std::nullopt;

    template<size_t I, typename Row,
        std::enable_if_t<std::is_same_v<q20::remove_cvref_t<Row>, TreeRow>, bool> = true>
    friend inline decltype(auto) get(Row &&row)
    {
        if constexpr (I == 0)
            return q23::forward_like<Row>(row.m_name);
        else if constexpr (I == 1)
            return q23::forward_like<Row>(row.m_title);
    }

    friend QDebug operator<<(QDebug dbg, const TreeRow &row)
    {
        dbg << "TreeRow(" << row.m_name << row.m_title << ")";
        return dbg;
    }
};

namespace std {
    template <> struct tuple_size<TreeRow> : std::integral_constant<size_t, 2> {};
    template <size_t I> struct tuple_element<I, TreeRow>
    { using type = decltype(get<I>(std::declval<TreeRow>())); };
}

template <>
struct QRangeModel::RowOptions<TreeRow>
{
    static QStringList mimeTypes() { return {u"application/json"_s}; }

    using Path = QVarLengthArray<int, 32>;
    static Path makePath(const QModelIndex &index) {
        // we call with col0 indexes and a parent can never be at another column
        Q_ASSERT(index.column() == 0);
        Path path{index.row()};
        QModelIndex parent = index.parent();
        while (parent.isValid()) {
            path.append(parent.row());
            parent = parent.parent();
        }
        std::reverse(path.begin(), path.end());
        return path;
    };

    static QJsonArray makeArray(const auto &rows)
    {
        QList<std::pair<QVarLengthArray<int, 32>, QJsonObject>> builtObjects;

        std::for_each(rows.rbegin(), rows.rend(), [&](const auto &entry) {
            const auto &[row, index] = entry;
            QJsonObject rowObject;
            if (index.column() == -1) { // full row selected
                rowObject.insert("name", get<0>(row));
                rowObject.insert("title", get<1>(row));
            } else {
                switch (index.column()) {
                case 0:
                    rowObject.insert("name", get<0>(row));
                    break;
                case 1:
                    rowObject.insert("title", get<1>(row));
                    break;
                }
            }
            Path rowPath = makePath(index.siblingAtColumn(0));

            QJsonArray children;
            for (auto entry = builtObjects.cbegin(); entry != builtObjects.cend();) {
                const auto &[builtPath, builtObject] = *entry;
                if (rowPath.size() < builtPath.size()) {
                    int i = 0;
                    while (i < std::min(builtPath.size(), rowPath.size()) && builtPath[i] == rowPath[i])
                        ++i;
                    if (i) {
                        children.append(builtObject);
                        entry = builtObjects.erase(entry);
                        continue;
                    }
                }
                ++entry;
            }
            if (!children.isEmpty())
                rowObject.insert("children", children);
            builtObjects.prepend({rowPath, rowObject});
        });

        QJsonArray result;
        for (const auto &[_, object] : builtObjects)
            result.append(object);
        return result;
    }

    static QMimeData *mimeData(const auto &rows)
    {
        if (rows.empty())
            return nullptr;

        QJsonDocument document(makeArray(rows));
        QByteArray data = document.toJson();

        QMimeData *mimeData = new QMimeData;
        mimeData->setData(mimeTypes().first(), data);
        return mimeData;
    }

    static bool dropMimeData(const QMimeData *data, auto inserter)
    {
        QByteArray json = data->data(mimeTypes().first());
        if (json.isEmpty())
            return false;
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(json, &error);
        if (error.error) {
            qWarning("Invalid JSON: %s at offset %d", qPrintable(error.errorString()), error.offset);
            return false;
        }
        QJsonArray topObjects = document.array();
        Tree newTree = TreeRow::fromJsonArray(topObjects);
        for (auto &newRow : newTree)
            inserter = std::move(newRow);
        return true;
    }
};

class Object : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString display READ display WRITE setDisplay NOTIFY displayChanged)
    Q_PROPERTY(QColor decoration READ decoration WRITE setDecoration NOTIFY decorationChanged)

public:
    Object(int x, QObject *parent = nullptr)
        : QObject(parent)
        , m_display(QString::number(x))
        , m_decoration(QColor::fromHsv(x % 255, 255, 255))
    {}
    QString display() const { return m_display; }
    void setDisplay(const QString &display)
    {
        if (m_display == display)
            return;
        m_display = display;
        emit displayChanged();
    }

    QColor decoration() const { return m_decoration; }
    void setDecoration(QColor color)
    {
        if (m_decoration == color)
            return;
        m_decoration = color;
        emit decorationChanged();
    }

Q_SIGNALS:
    void displayChanged();
    void decorationChanged();

private:
    QString m_display;
    QColor m_decoration;
};

class ModelFactory : public QObject
{
    Q_OBJECT

    std::vector<int> numbers = {1, 10, 3, 8, 5, 6, 7, 4, 9, 2, 11};
    QList<QString> strings = {u"one"_s, u"two"_s, u"three"_s};
    std::array<int, 1000000> largeArray = {};
    QTimer updater;

public:
    void reset()
    {
        updater.stop();
        updater.disconnect();
    }

public slots:
    QRangeModel *makeNumbers()
    {
        return new QRangeModel(&numbers);
    }

    QRangeModel *makeDiacritics()
    {
        auto *model = new QRangeModel(QStringList{
            u"Foie gras"_s,
            u"Éclair"_s,
            u"Pâté"_s,
            u"Pain"_s,
            u"Crème brûlée"_s,
            u"Crêpe"_s,
            u"Crevette"_s,
            u"Soupe à l'oignon"_s,
            u"Soupe aux amandes"_s,
            u"Bûche de Noël"_s,
            u"Bugne"_s,
            u"Gâteau"_s,
            u"Galette"_s,
            u"Chèvre"_s,
            u"Chevreuil"_s,
            u"Macaron"_s,
            u"Maïs"_s,
        });
        return model;
    }

    QRangeModel *makeLargeArray()
    {
        for (int i = 0; i < int(largeArray.size()); ++i)
            largeArray[i] = i;
        return new QRangeModel(&largeArray);
    }

    QRangeModel *makeVectorWithAdapter()
    {
        QRangeModelAdapter adapter(std::ref(numbers));
        auto *model = adapter.model();
        qDebug() << "Data from index" << adapter.index(0).data();
        qDebug() << "Data from adapter" << adapter.data(0);
        qDebug() << "Data from operator[]" << adapter[0];

        connect(&updater, &QTimer::timeout, model, [adapter] mutable {
            adapter[0] = adapter[0] + 1;
        });

        updater.start(1000);
        return model;
    }

    QRangeModel *makeStrings()
    {
        return new QRangeModel(std::ref(strings));
    }

    QRangeModel *makeJson()
    {
        QJsonDocument json = QJsonDocument::fromJson(R"(
            [ "one", "two", 12345 ]
        )");
        Q_ASSERT(json.isArray());
        return new QRangeModel(json.array());
    }

    QRangeModel *makeListOfTuples()
    {
        std::list<std::tuple<int, QString>> data = {
            { 1, "eins"},
            { 2, "zwei"},
            { 3, "drei"},
            { 4, "vier"},
            { 5, "fünf"},
        };

        return new QRangeModel(data);
    }

    QRangeModel *makeListOfArrays()
    {
        QList<std::array<double, 2000>> data = {
            {0.0},
            {1.1},
            {2.2},
        };

        return new QRangeModel(std::move(data));
    }

    QRangeModel *makeCustomFromEnum()
    {
        return new QRangeModel(QMetaEnumerator(Qt::ItemDataRole{}));
    }

    QRangeModel *makeBoundedIota()
    {
#ifdef __cpp_lib_ranges
        return new QRangeModel(std::views::iota(1, 10000));
#else
        return new QRangeModel(QList{u"Not available: std::views requires C++20 ranges support"
                                     " (platform libc++ has disabled incomplete ranges)"_s});
#endif
    }

    QRangeModel *makeUnboundedIota()
    {
#ifdef __cpp_lib_ranges
        auto view = std::views::iota(1);
        return new QRangeModel(view);
#else
        return new QRangeModel(QList{u"Not available: std::views requires C++20 ranges support"
                                     " (platform libc++ has disabled incomplete ranges)"_s});
#endif
    }

    QRangeModel *makeZipView()
    {
#ifdef __cpp_lib_ranges_zip
        static auto x = QList<int>{1, 2, 3, 4, 5};
        static auto y = std::list<QString>{"α", "β", "γ", "δ", "ε"};
        static auto z = std::array<QChar, 6>{u'A', u'B', u'C', u'D', u'E', u'F'};

        return new QRangeModel(std::views::zip(x, y, z));
#else
        return new QRangeModel(QList{u"Not available: std::views::zip requires C++23 ranges support"_s});
#endif
    }

    QRangeModel *makeGadgetList()
    {
        QList<Gadget> gadgetList = {
            {"1/1", Qt::red, "red"},
            {"1/2", Qt::black, "black"},
            {"2/1", Qt::blue, "blue"},
            {"2/2", Qt::green, "green"},
        };
        return new QRangeModel(gadgetList);
    }

    QRangeModel *makeGadgetPointerList()
    {
        std::vector<std::unique_ptr<Gadget>> gadgetList;
        std::array gadgets = {
            std::make_unique<Gadget>("1/1", Qt::red, "red"),
            std::make_unique<Gadget>("1/2", Qt::black, "black"),
            std::unique_ptr<Gadget>(),
            std::make_unique<Gadget>("3/1", Qt::blue, "blue"),
            std::make_unique<Gadget>("3/2", Qt::green, "green"),
        };
        std::copy(std::move_iterator(gadgets.begin()), std::move_iterator(gadgets.end()),
                  std::back_inserter(gadgetList));
        return new QRangeModel(std::move(gadgetList));
    }

    QRangeModel *makeGadgetTable()
    {
        QList<QList<Gadget>> gadgetTable = {
            {{"1/1", Qt::red, "red"}, {"1/2", Qt::black, "black"}},
            {{"2/1", Qt::blue, "blue"}, {"2/2", Qt::green, "green"}},
        };
        return new QRangeModel(gadgetTable);
    }

   QRangeModel *makeGadgetPointerTable()
    {
        QList<std::vector<std::shared_ptr<Gadget>>> gadgetTable = {
            {
                std::make_shared<Gadget>("1/1", Qt::red, "red"),
                std::make_shared<Gadget>("1/2", Qt::black, "black")
            },
            {
                std::shared_ptr<Gadget>(), std::make_shared<Gadget>("2/1", Qt::blue, "blue"),
            },
            {
                std::make_shared<Gadget>("3/1", Qt::blue, "blue"),
                std::make_shared<Gadget>("3/2", Qt::green, "green")
            },
        };
        return new QRangeModel(gadgetTable);
    }

    QRangeModel *makeQtMap()
    {
        return new QRangeModel(QMap<QString, QString>{
            {"one", "eins"},
            {"two", "zwei"},
            {"three", "drei"},
            {"four", "vier"},
        });
    }

    QRangeModel *makeStdMap()
    {
        return new QRangeModel(std::map<int, double>{
            {1, 0.1},
            {2, 0.2},
            {3, 0.3},
            {4, 0.4},
        });
    }

    QRangeModel *makeFilterView()
    {
#ifdef __cpp_lib_ranges
        const QDate today = QDate::currentDate();
        auto view = std::views::iota(today.addYears(-100), today.addYears(100))
                  | std::views::filter([](QDate date){
                        return date.dayOfWeek() < 6;
                    });

        return new QRangeModel(view);
#else
        return new QRangeModel(QList{u"Not available: std::views requires C++20 ranges support"
                                     " (platform libc++ has disabled incomplete ranges)"_s});
#endif
    }

    QRangeModel *makeMultiRoleMap()
    {
        using ColorEntry = QMap<Qt::ItemDataRole, QVariant>;

        const QStringList colorNames = QColor::colorNames();
        QList<ColorEntry> colors;
        colors.reserve(colorNames.size());
        for (const QString &name : QColor::colorNames()) {
            const QColor color = QColor::fromString(name);
            colors << ColorEntry{{Qt::DisplayRole, name},
                                {Qt::DecorationRole, color},
                                {Qt::ToolTipRole, color.name()}};
        }
        return new QRangeModel(colors);
    }

    QRangeModel *makeUniqueObjects()
    {
        std::array<std::unique_ptr<Object>, 4> data = {
            std::make_unique<Object>(1),
            std::make_unique<Object>(2),
            std::unique_ptr<Object>(),
            std::make_unique<Object>(3),
        };
        return new QRangeModel(std::move(data));
    }

    QRangeModel *makeUniquePtrArray()
    {
        // not possible, values need to be copyable
        // std::array<std::unique_ptr<QString>, 3> data = {
        //     std::make_unique<QString>("A"),
        //     std::make_unique<QString>("B"),
        //     std::make_unique<QString>("C"),
        // };
        // return new QRangeModel(std::move(data));
        return new QRangeModel(QList{"Nothing to see here"});
    }

    QRangeModel *makeUniqueRows()
    {
        std::array<std::unique_ptr<std::vector<QString>>, 3> data = {
            std::make_unique<std::vector<QString>>(std::vector<QString>{u"A"_s, u"B"_s, u"C"_s}),
            std::make_unique<std::vector<QString>>(std::vector<QString>{u"D"_s, u"E"_s, u"F"_s}),
            std::make_unique<std::vector<QString>>(std::vector<QString>{u"G"_s, u"H"_s, u"I"_s}),
        };
        return new QRangeModel(std::move(data));
    }

    QRangeModel *makeTree()
    {
        TreeRow root[] = {{"Germany", "Berlin"},
                          {"France", "Paris"},
                          {"Austria", "Vienna"}
                         };

        Tree europe{std::make_move_iterator(std::begin(root)),
                    std::make_move_iterator(std::end(root))};
        TreeRow &bavaria = europe[0].addChild("Bavaria", "Munich");
        bavaria.addChild("Upper Bavaria", "München");
        bavaria.addChild("Lower Bavaria", "Landshut");
        bavaria.addChild("Upper Palatinate", "Regensburg");
        bavaria.addChild("Swabia", "Augsburg");
        bavaria.addChild("Franconia", "Nürnberg");
        bavaria.addChild("Upper Franconia", "Bayreuth");
        bavaria.addChild("Middle Franconia", "Ansbach");
        bavaria.addChild("Lower Franconia", "Würzburg");

        TreeRow &hessia = europe[0].addChild("Hessia", "Wiesbaden");
        hessia.addChild("Upper Hesse", "Giessen");
        hessia.addChild("Lower Hesse", "Darmstadt");
        hessia.addChild("North Hesse", "Kassel");

        europe[1].addChild("Île-de-France", "Paris");
        europe[1].addChild("Provence-Alpes-Côte d'Azur", "Marseille");
        europe[1].addChild("Auvergne-Rhône-Alpes", "Lyon");
        europe[1].addChild("Nouvelle-Aquitaine", "Bordeaux");

        europe[2].addChild("Vienna", "Vienna");
        europe[2].addChild("Lower Austria", "St. Pölten");
        europe[2].addChild("Upper Austria", "Linz");
        europe[2].addChild("Styria", "Graz");
        europe[2].addChild("Carinthia", "Klagenfurt");
        europe[2].addChild("Salzburg", "Salzburg");
        europe[2].addChild("Tyrol", "Innsbruck");
        europe[2].addChild("Vorarlberg", "Bregenz");
        europe[2].addChild("Burgenland", "Eisenstadt");

        QRangeModelAdapter adapter(std::move(europe));
        const QList<int> path = {1, 0};
        QRangeModel *model = adapter.model();
        connect(&updater, &QTimer::timeout, model, [adapter] mutable {
            for (auto row : adapter)
                qDebug() << row[0] << row[1];
        });
        updater.start(1000);

        return model;
    }

    QRangeModel *makeAutoConnectedObjects()
    {
        std::array<std::pair<Object *, Object *>, 500> table;
        for (size_t i = 0; i < table.size(); ++i) {
            table[i].first = new Object(i);
            table[i].second = new Object(i);
        }

        QRangeModelAdapter adapter(std::move(table));
        QRangeModel *model = adapter.model();
        connect(&updater, &QTimer::timeout, this, [adapter = std::move(adapter)] {
            for (auto row : adapter) {
                for (auto column : row) {
                    const_cast<Object *>(column)->setDisplay(QTime::currentTime().toString());
                }
            }
        });

        updater.start(1000);
        model->setAutoConnectPolicy(QRangeModel::AutoConnectPolicy::OnRead);
        return model;
    }

    QRangeModel *makeAutoConnectedConstObjects()
    {
        std::array<std::pair<Object *, Object *>, 500> table;
        for (size_t i = 0; i < table.size(); ++i) {
            table[i].first = new Object(i);
            table[i].second = new Object(i);
        }

        QRangeModelAdapter adapter(std::move(std::as_const(table)));
        QRangeModel *model = adapter.model();
        connect(&updater, &QTimer::timeout, this, [adapter = std::move(adapter)] {
            for (auto row : adapter) {
                for (auto column : row) {
                    auto object = const_cast<Object *>(column);
                    object->setDisplay(QTime::currentTime().toString());
                    int hue = column->decoration().hue() + 1;
                    object->setDecoration(QColor::fromHsv(hue % 255, 255, 255));
                }
            }
        });

        updater.start(1000);
        model->setAutoConnectPolicy(QRangeModel::AutoConnectPolicy::Full);
        return model;
    }

    QRangeModel *makeAutoConnectedRows()
    {
        QList<Object *> list;
        for (int i = 0; i < 100; ++i)
            list.append(new Object(i));

        QRangeModelAdapter adapter(std::move(list));
        QRangeModel *model = adapter.model();
        connect(&updater, &QTimer::timeout, this, [adapter = std::move(adapter)] {
            for (auto row : adapter) {
                auto object = const_cast<Object *>(row.get());
                object->setDisplay(QTime::currentTime().toString());
                int hue = row->decoration().hue() + 1;
                object->setDecoration(QColor::fromHsv(hue % 255, 255, 255));
            }
        });

        updater.start(100);
        model->setAutoConnectPolicy(QRangeModel::AutoConnectPolicy::Full);
        return model;
    }
};

struct QMetaMethodEnumerator
{
    struct iterator
    {
        using difference_type = int;
        using size_type = int;
        using pointer = void;
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::tuple<int, QByteArray, QMetaMethod>;
        using reference = value_type;
        using const_reference = const value_type;

        friend constexpr iterator &operator++(iterator &that)
        { ++that.m_index; return that; }
        friend constexpr iterator operator++(iterator &that, int)
        { auto copy = that; ++that.m_index; return copy; }
        friend constexpr iterator &operator+=(iterator &that, int n)
        { that.m_index += n; return that; }

        friend constexpr bool comparesEqual(const iterator &lhs, const iterator &rhs) noexcept
        {
            return lhs.m_index == rhs.m_index && lhs.m_metaobject == rhs.m_metaobject;
        }

        friend constexpr Qt::strong_ordering compareThreeWay(const iterator &lhs,
                                                             const iterator &rhs) noexcept
        {
            return Qt::compareThreeWay(lhs.m_index, rhs.m_index);
        }
        Q_DECLARE_STRONGLY_ORDERED(iterator)

        const_reference operator*() const
        { return {m_index,
                  m_metaobject->method(m_index).name().slice(4),
                  m_metaobject->method(m_index)}; }

        const QMetaObject *m_metaobject = nullptr;
        int m_index = 0;
    };

    static_assert(std::input_iterator<iterator>);

    using size_type = iterator::size_type;
    using value_type = iterator::value_type;
    using const_iterator = iterator;

    const_iterator begin() const { return iterator{&m_metaobject, m_metaobject.methodOffset()}; }
    const_iterator end() const { return iterator{&m_metaobject, size()}; }
    size_type size() const { return m_metaobject.methodCount() - m_metaobject.methodOffset(); }

    explicit QMetaMethodEnumerator(const QMetaObject &mo) noexcept
        : m_metaobject(mo)
    {}

    template <typename Class>
    static QMetaMethodEnumerator fromType() noexcept
    {
        return QMetaMethodEnumerator{Class::staticMetaObject};
    }

private:
    const QMetaObject &m_metaobject;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr)
        : QMainWindow(parent), model(nullptr)
    {
        QSplitter *splitter = new QSplitter;
        QSplitter *viewSplitter = new QSplitter(Qt::Vertical);

        treeview = new QTreeView;
        treeview->setSelectionMode(QAbstractItemView::ExtendedSelection);
        treeview->setUniformRowHeights(true);
        treeview->setDragEnabled(true);
        treeview->setDropIndicatorShown(true);
        treeview->setAcceptDrops(true);
        treeview->viewport()->setAcceptDrops(true);
        treeview->setDragDropMode(QAbstractItemView::DragDrop);
        viewSplitter->addWidget(treeview);

        tableview = new QTableView;
        tableview->setDragEnabled(true);
        tableview->setDropIndicatorShown(true);
        tableview->setAcceptDrops(true);
        tableview->viewport()->setAcceptDrops(true);
        tableview->setDragDropOverwriteMode(true);
        tableview->setDragDropMode(QAbstractItemView::DragDrop);
        viewSplitter->addWidget(tableview);

        splitter->addWidget(viewSplitter);

#ifdef QUICK_UI
        quickWidget = new QQuickWidget;
        connect(quickWidget, &QQuickWidget::statusChanged, this, [](QQuickWidget::Status status){
            qDebug() << "Quick UI status" << status;
        });
        quickWidget->loadFromModule("Main", "Main");
        quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
        splitter->addWidget(quickWidget);
#endif

        setCentralWidget(splitter);

        QComboBox *modelPicker = new QComboBox;
        connect(modelPicker, &QComboBox::currentIndexChanged, this, &MainWindow::modelChanged);

        QToolBar *toolBar = addToolBar(tr("Model Operations"));
        toolBar->addWidget(modelPicker);

        toolBar->addSeparator();

        QAction *sortAction = toolBar->addAction(tr("Sort"));
        sortAction->setCheckable(true);
        connect(sortAction, &QAction::toggled, treeview, &QTreeView::setSortingEnabled);

        QMenu *collatorOptions = new QMenu(this);
        QAction *ignorePunctuationOption = collatorOptions->addAction(tr("IgnorePunctuation"));
        ignorePunctuationOption->setCheckable(true);
        ignorePunctuationOption->setData(QVariant::fromValue(QCollator::CollationOption::IgnorePunctuation));
        QAction *diacriticInsensitiveOption = collatorOptions->addAction(tr("DiacriticInsensitive"));
        diacriticInsensitiveOption->setCheckable(true);
        diacriticInsensitiveOption->setData(QVariant::fromValue(QCollator::CollationOption::DiacriticInsensitive));
        matchAction = toolBar->addAction(tr("Match"));
        connect(matchAction, &QAction::toggled, this, [this](bool checked){
            if (checked)
                model->setMatchCollator(matchCollator);
            else
                model->resetMatchCollator();
        });
        connect(collatorOptions, &QMenu::triggered, this, [this](QAction *action){
            auto options = matchCollator.options();
            options.setFlag(action->data().value<QCollator::CollationOption>(), action->isChecked());
            matchCollator.setOptions(options);
            if (matchAction->isChecked())
                model->setMatchCollator(matchCollator);
        });

        matchAction->setCheckable(true);
        matchAction->setMenu(collatorOptions);

        toolBar->addSeparator();

        QAction *addAction = toolBar->addAction(tr("Add"), this, &MainWindow::onAdd);
        addAction->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::ListAdd));
        QAction *removeAction = toolBar->addAction(tr("Remove"), this, &MainWindow::onRemove);
        removeAction->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::ListRemove));
        QAction *upAction = toolBar->addAction(tr("Move up"), this, &MainWindow::onUp);
        upAction->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::GoUp));
        QAction *downAction = toolBar->addAction(tr("Move down"), this, &MainWindow::onDown);
        downAction->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::GoDown));
        QAction *indentAction = toolBar->addAction(tr("Move in"), this, &MainWindow::onIn);
        indentAction->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::FormatIndentMore));
        QAction *dedentAction = toolBar->addAction(tr("Move out"), this, &MainWindow::onOut);
        dedentAction->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::FormatIndentLess));

        toolBar->addSeparator();

        connectionOptions = new QActionGroup(this);
        QAction *fullAutoConnectAction = connectionOptions->addAction(tr("Full"));
        fullAutoConnectAction->setCheckable(true);
        fullAutoConnectAction->setData(QVariant::fromValue(QRangeModel::AutoConnectPolicy::Full));
        QAction *onReadAutoConnectAction = connectionOptions->addAction(tr("On Read"));
        onReadAutoConnectAction->setCheckable(true);
        onReadAutoConnectAction->setData(QVariant::fromValue(QRangeModel::AutoConnectPolicy::OnRead));

        connectionOptions->setExclusive(true);
        connectionOptions->setExclusionPolicy(QActionGroup::ExclusionPolicy::ExclusiveOptional);

        toolBar->addAction(fullAutoConnectAction);
        toolBar->addAction(onReadAutoConnectAction);

        connect(connectionOptions, &QActionGroup::triggered, this, [this](){
            QAction *checkedAction = connectionOptions->checkedAction();
            const auto policy = checkedAction ? checkedAction->data().value<QRangeModel::AutoConnectPolicy>()
                                              : QRangeModel::AutoConnectPolicy::None;

            qDebug() << "Switching to policy" << policy;
            model->setAutoConnectPolicy(policy);
        });

        // this will implicitly run modelChanged() and update the UI
        modelPicker->setModel(new QRangeModel(QMetaMethodEnumerator::fromType<ModelFactory>(),
                                              modelPicker));
        modelPicker->setModelColumn(1);
    }

    ~MainWindow()
    {
        factory.reset();
    }

private:
    void modelChanged(int index)
    {
        QPointer<QRangeModel> oldModel = model;

        factory.reset();

        QRangeModel *newModel = nullptr;
        const QMetaObject &mo = ModelFactory::staticMetaObject;
        const QMetaMethod method = mo.method(index + mo.methodOffset());
        if (method.invoke(&factory, qReturnArg(newModel))) {
            model = newModel;
            newModel->setParent(this);
            newModel->setObjectName(QString::fromUtf8(method.name()).slice(4));
            treeview->setModel(newModel);
            tableview->setModel(newModel);
            newModel->setSupportedDragActions(Qt::CopyAction | Qt::MoveAction);
            newModel->setSupportedDropActions(Qt::CopyAction | Qt::MoveAction);
#ifdef QUICK_UI
            if (!quickWidget->rootObject())
                statusBar()->showMessage(tr("Failed to load QML"));
            else
                quickWidget->rootObject()->setProperty("model", QVariant::fromValue(newModel));
#endif
            for (auto *action : connectionOptions->actions()) {
                action->setChecked(action->data().value<QRangeModel::AutoConnectPolicy>()
                                == model->autoConnectPolicy());
            }

            if (matchAction->isChecked())
                model->setMatchCollator(matchCollator);
        }

        delete oldModel;
    }

    void onAdd()
    {
        const auto current = treeview->currentIndex();
        showMessage(tr("Inserting after '%1'").arg(current.data().toString()));
        if (!model->insertRows(current.row() + 1, 1, current.parent())) {
            showMessage(tr("Insertion failed"));
        } else {
            const auto newIndex = model->index(current.row() + 1, 0, current.parent());
            static int counter = 0;
            model->setData(newIndex, u"New Value %1"_s.arg(++counter));
        }
    }

    void onRemove()
    {
        const auto current = treeview->currentIndex();
        showMessage(tr("Removing '%1'").arg(current.data().toString()));
        if (!model->removeRows(current.row(), 1, model->parent(current)))
            showMessage(tr("Removal failed"));
    }

    void onUp()
    {
        const auto current = treeview->currentIndex();
        showMessage(tr("Moving '%1' up").arg(current.data().toString()));
        const auto currentParent = current.parent();
        if (!model->moveRows(currentParent, current.row(), 1, currentParent, current.row() - 1))
            showMessage(tr("Failed to move up"));
    }

    void onDown()
    {
        const auto current = treeview->currentIndex();
        showMessage(tr("Moving '%1' down").arg(current.data().toString()));
        const auto currentParent = current.parent();
        if (!model->moveRows(currentParent, current.row(), 1, currentParent, current.row() + 2))
            showMessage(tr("Failed to move down"));
    }

    void onIn()
    {
        const auto current = treeview->currentIndex();
        showMessage(tr("Moving '%1' in").arg(current.data().toString()));
        const auto currentParent = current.parent();
        const auto newParent = current.sibling(current.row() - 1, 0);
        // move the selected row under it's top-most sibling

        if (!model->moveRows(currentParent, current.row(), 1, newParent, model->rowCount(newParent)))
            showMessage(tr("Indentation failed"));
    }

    void onOut()
    {
        const auto current = treeview->currentIndex();
        showMessage(tr("Moving '%1' out").arg(current.data().toString()));
        const auto currentParent = current.parent();
        const auto grandParent = currentParent.parent();
        // move the selected row under it's grandparent
        if (!model->moveRows(currentParent, current.row(), 1, grandParent, currentParent.row()))
            showMessage(tr("Dedentation failed"));
    }

    void showMessage(const QString &message, int timeout = 2000)
    {
        statusBar()->showMessage(message, timeout);
    }

    ModelFactory factory;
    QPointer<QRangeModel> model; // might be owned by an adapter
    QTreeView *treeview;
    QTableView *tableview;
#ifdef QUICK_UI
    QQuickWidget *quickWidget;
#endif
    QActionGroup *connectionOptions;
    QAction *matchAction;
    QCollator matchCollator;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    auto mainWindow = std::make_unique<MainWindow>();
    mainWindow->show();

    return app.exec();
}

#include "main.moc"
