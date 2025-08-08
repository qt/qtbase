// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore>
#include <QtWidgets>

#if __has_include(<QtQml>)
#define QUICK_UI
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>
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
    QString m_display;
    QColor m_decoration;
    QString m_toolTip;
};

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
    explicit QMetaEnumerator(Enum e) noexcept
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
        if (!m_children)
            m_children.emplace(Tree{});
        TreeRow &res = m_children->emplace_back(args...);
        res.m_parent = this;
        return res;
    }

    // tree traversal protocol implementation
    const TreeRow *parentRow() const { return m_parent; }
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
};

namespace std {
    template <> struct tuple_size<TreeRow> : std::integral_constant<size_t, 2> {};
    template <size_t I> struct tuple_element<I, TreeRow>
    { using type = decltype(get<I>(std::declval<TreeRow>())); };
}


class Object : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString display READ display WRITE setDisplay NOTIFY displayChanged)

public:
    Object(int x)
        : m_display(QString::number(x))
    {}
    QString display() const { return m_display; }
    void setDisplay(const QString &d) { m_display = d; displayChanged(); }

Q_SIGNALS:
    void displayChanged();

private:
    QString m_display;
};

template <>
struct QRangeModel::RowOptions<Object>
{
    static constexpr auto rowCategory = QRangeModel::RowCategory::MultiRoleItem;
};

class ModelFactory : public QObject
{
    Q_OBJECT

    std::vector<int> numbers = {1, 2, 3, 4, 5};
    QList<QString> strings = {u"one"_s, u"two"_s, u"three"_s};

public slots:
    QRangeModel *makeNumbers()
    {
        return new QRangeModel(&numbers);
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

    QRangeModel *makeCustomFromEnum()
    {
        return new QRangeModel(QMetaEnumerator(Qt::ItemDataRole{}));
    }

    QRangeModel *makeBoundedIota()
    {
        return new QRangeModel(std::views::iota(1, 10000));
    }

    QRangeModel *makeUnboundedIota()
    {
        auto view = std::views::iota(1);
        return new QRangeModel(view);
    }

    QRangeModel *makeZipView()
    {
        static auto x = QList<int>{1, 2, 3, 4, 5};
        static auto y = std::list<QString>{"α", "β", "γ", "δ", "ε"};
        static auto z = std::array<QChar, 6>{u'A', u'B', u'C', u'D', u'E', u'F'};

        return new QRangeModel(std::views::zip(x, y, z));
    }

    QRangeModel *makeGadgetTable()
    {
        QList<QList<Gadget>> gadgetTable = {
            {{"1/1", Qt::red, "red"}, {"1/2", Qt::black, "black"}},
            {{"2/1", Qt::blue, "blue"}, {"2/2", Qt::green, "green"}},
        };
        return new QRangeModel(gadgetTable);
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
        std::array<std::unique_ptr<Object>, 3> data = {
            std::make_unique<Object>(1),
            std::make_unique<Object>(2),
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
        return nullptr;
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
        static TreeRow root[] = {{"Germany", "Berlin"},
                                 {"France", "Paris"},
                                 {"Austria", "Vienna"}
                                };

        static Tree europe{std::make_move_iterator(std::begin(root)), std::make_move_iterator(std::end(root))};
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

        return new QRangeModel(std::ref(europe));
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

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

#ifdef QUICK_UI
    QQmlApplicationEngine engine;
    engine.loadFromModule("Main", "Main");
#endif

    QWidget widget;
    widget.setWindowTitle("Qt Widgets");
    QTreeView treeview;
    treeview.setUniformRowHeights(true);
    QToolBar toolbar;
    QComboBox modelPicker;
    ModelFactory factory;

    QObject::connect(&modelPicker, &QComboBox::currentIndexChanged, &modelPicker, [&treeview,
#ifdef QUICK_UI
                     rootObjects = engine.rootObjects(),
#endif
                     &factory](int index){
        const QMetaObject &mo = ModelFactory::staticMetaObject;
        const QMetaMethod method = mo.method(index + mo.methodOffset());
        if (QRangeModel *newModel; method.invoke(&factory, qReturnArg(newModel))) {
            QAbstractItemModel *oldModel = treeview.model();
            treeview.setModel(newModel);
#ifdef QUICK_UI
            rootObjects.at(0)->setProperty("model", QVariant::fromValue(newModel));
#endif
            delete oldModel;
        }
    });

    modelPicker.setModel(new QRangeModel(QMetaMethodEnumerator::fromType<ModelFactory>(),
                                         &modelPicker));
    modelPicker.setModelColumn(1);
    toolbar.addWidget(&modelPicker);
    QAction *addAction = toolbar.addAction("+", &treeview, [&treeview]{
        auto *model = treeview.model();
        const auto current = treeview.currentIndex();
        if (!model->insertRows(current.row() + 1, 1, current.parent())) {
            qDebug() << "Insertion failed";
        } else {
            const auto newIndex = model->index(current.row() + 1, 0, current.parent());
            static int counter = 0;
            model->setData(newIndex, u"New Value %1"_s.arg(++counter));
        }
    });
    QAction *removeAction = toolbar.addAction("-", &treeview, [&treeview]{
        auto *model = treeview.model();
        const auto current = treeview.currentIndex();
        qDebug() << "Removing" << current.data();
        if (!model->removeRows(current.row(), 1, model->parent(current)))
            qDebug() << "Removal failed";
    });
    QAction *upAction = toolbar.addAction("^", &treeview, [&treeview]{
        auto *model = treeview.model();
        const auto current = treeview.currentIndex();
        qDebug() << "Moving up" << current.data();
        const auto currentParent = current.parent();
        if (!model->moveRows(currentParent, current.row(), 1, currentParent, current.row() - 1))
            qDebug() << "Failed to move up";
    });
    QAction *downAction = toolbar.addAction("v", &treeview, [&treeview]{
        auto *model = treeview.model();
        const auto current = treeview.currentIndex();
        qDebug() << "Moving down" << current.data();
        const auto currentParent = current.parent();
        if (!model->moveRows(currentParent, current.row(), 1, currentParent, current.row() + 2))
            qDebug() << "Failed to move down";
    });
    QAction *indentAction = toolbar.addAction(">", &treeview, [&treeview]{
        auto *model = treeview.model();
        const auto current = treeview.currentIndex();
        qDebug() << "Moving in" << current.data();
        const auto currentParent = current.parent();
        const auto newParent = current.sibling(current.row() - 1, 0);
        // move the selected row under it's top-most sibling

        if (!model->moveRows(currentParent, current.row(), 1, newParent, model->rowCount(newParent)))
            qDebug() << "Indentation failed";
    });
    QAction *dedentAction = toolbar.addAction("<", &treeview, [&treeview]{
        auto *model = treeview.model();
        const auto current = treeview.currentIndex();
        qDebug() << "Moving out" << current.data();
        const auto currentParent = current.parent();
        const auto grandParent = currentParent.parent();
        // move the selected row under it's grandparent
        QPersistentModelIndex guard = model->index(current.row(), 0, currentParent);
        QPersistentModelIndex watch = model->index(current.row() - 1, 0, currentParent);
        if (!model->moveRows(currentParent, current.row(), 1, grandParent, currentParent.row()))
            qDebug() << "Dedentation failed";
    });

    QVBoxLayout vbox;
    vbox.addWidget(&treeview);
    vbox.addWidget(&toolbar);

    widget.setLayout(&vbox);
    widget.show();
    int res = app.exec();

    return res;
}

#include "main.moc"
