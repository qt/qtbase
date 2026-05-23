// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtCore/qrangemodel.h>

#ifndef QT_NO_WIDGETS

#include <QtCore/qiodevice.h>
#include <QtCore/qmimedata.h>
#include <QtCore/qstring.h>
#include <QtCore/qtextstream.h>
#include <QtCore/qxmlstream.h>
#include <QtGui/qcolor.h>

#include <QtWidgets/qlistview.h>
#include <QtWidgets/qtableview.h>

using namespace Qt::StringLiterals;

namespace multirole_gadget {
//! [color_gadget_decl]
class ColorEntry
{
    Q_GADGET
    Q_PROPERTY(QString display READ colorName)
    Q_PROPERTY(QColor decoration READ decoration)
    Q_PROPERTY(QString toolTip READ toolTip)
public:
//! [color_gadget_decl]
    Qt::ItemFlags flags() const { return {}; }
//! [color_gadget_impl]
    ColorEntry(const QString &color = {})
        : m_colorName(color)
    {}

    QString colorName() const
    {
        return m_colorName;
    }
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
//! [color_gadget_row_options_decl]

template <>
struct QRangeModel::RowOptions<ColorEntry>
{
//! [color_gadget_row_options_decl]
//! [color_gadget_row_options_rowCategory]
    static constexpr auto rowCategory = QRangeModel::RowCategory::MultiRoleItem;
//! [color_gadget_row_options_rowCategory]
//! [color_gadget_row_options_headerData]
    static QVariant headerData(int section, int role)
    {
        switch (section) {
            // ...
        }
        return {};
    }
//! [color_gadget_row_options_headerData]
//! [color_gadget_row_options_flags]
    static Qt::ItemFlags flags(const ColorEntry &entry)
    {
        return entry.flags();
    }
//! [color_gadget_row_options_flags]
//! [color_gadget_row_options_mimeTypes]
    static QStringList mimeTypes()
    {
        return {
            u"text/html"_s
        };
    }
//! [color_gadget_row_options_mimeTypes]
//! [color_gadget_row_options_mimeData]
    template <typename Items>
    static QMimeData *mimeData(const Items &items)
    {
        if (items.isEmpty())
            return nullptr;
        QByteArray data;
        QXmlStreamWriter stream(&data);
        stream.writeStartElement("ul");
        for (const auto &[item, index] : items)
            stream.writeTextElement("li", item.colorName());
        stream.writeEndElement();

        QMimeData *mimeData = new QMimeData;
        mimeData->setData(mimeTypes().first(), data);
        return mimeData;
    }
//! [color_gadget_row_options_mimeData]
//! [color_gadget_row_options_dropMimeData]
    static bool dropMimeData(const QMimeData *mimeData, auto inserter)
    {
        const QByteArray data = mimeData->data(mimeTypes().first());
        if (data.isEmpty())
            return false;
        QXmlStreamReader stream(data);
        while (!stream.atEnd()) {
            stream.readNext();
            if (stream.isStartElement() && stream.name() == u"li"_s)
                inserter = ColorEntry(stream.readElementText());
        }

        return true;
    }
//! [color_gadget_row_options_dropMimeData]
//! [color_gadget_row_options_end_decl]
};
//! [color_gadget_row_options_end_decl]
//! [color_gadget_row_options]

//! [color_gadget_item_access_decl]
template <>
struct QRangeModel::ItemAccess<ColorEntry>
{
//! [color_gadget_item_access_decl]
//! [color_gadget_item_access_flags]
    static Qt::ItemFlags flags(const ColorEntry &entry)
    {
        return entry.flags();
    }
//! [color_gadget_item_access_flags]
//! [color_gadget_item_access_readRole]
    static QVariant readRole(const ColorEntry &entry, int role)
    {
        switch (role) {
            // ...
        }
        return {};
    }
//! [color_gadget_item_access_readRole]
//! [color_gadget_item_access_writeRole]
    static bool writeRole(ColorEntry &entry, const QVariant &value, int role)
    {
        bool ok = false;
        switch (role) {
            // ...
        }
        return ok;
    }
//! [color_gadget_item_access_writeRole]
//! [color_gadget_item_access_mimeTypes]
    static QStringList mimeTypes()
    {
        return {
            u"text/plain"_s,
            u"application/x-qabstractitemmodeldatalist"_s
        };
    }
//! [color_gadget_item_access_mimeTypes]
//! [color_gadget_item_access_mimeData]
    static QMimeData *mimeData(const auto &range)
    {
        QByteArray data;
        QTextStream stream(&data, QIODevice::WriteOnly);
        for (const auto &[item, index] : range)
            stream << item.colorName() << Qt::endl;

        QMimeData *result = new QMimeData;
        result->setData(mimeTypes().first(), data);
        // Qt handles encoding into the default mime type
        return result;
    }
//! [color_gadget_item_access_mimeData]
//! [color_gadget_item_access_dropMimeData]
    static bool dropMimeData(const QMimeData *mimeData, auto inserter)
    {
        QByteArray data = mimeData->data(mimeTypes().first());
        if (data.isEmpty())
            return false;
        QTextStream stream(&data, QIODevice::ReadOnly);
        while (!stream.atEnd()) {
            QString colorName;
            stream >> colorName;
            inserter = ColorEntry(colorName);
        }
        return true;
    }
//! [color_gadget_item_access_dropMimeData]
//! [color_gadget_item_access_end_decl]
};
//! [color_gadget_item_access_end_decl]

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

void color_list_pointers()
{
    //! [color_gadget_item_access_use]

    std::vector<std::shared_ptr<ColorEntry>> colors;
    // ...
    QRangeModel model(colors);
    //! [color_gadget_item_access_use]
};

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

#endif //QT_NO_WIDGETS
