// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qstringlist.h>
#include <QtGui/private/qinternalmimedata_p.h>
#include <optional>
#include <qohosmimedata.h>
#include <utility>

QT_BEGIN_NAMESPACE

namespace {

template<typename T>
QOhosSupplier<T> makeMemoizingSupplier(QOhosSupplier<T> baseSupplier)
{
    return [baseSupplier = std::move(baseSupplier), value = std::optional<T>()]() mutable {
        if (!value.has_value()) {
            value.emplace(baseSupplier());
            baseSupplier = nullptr;
        }
        return value.value();
    };
}

class SuppliersBasedMimeData : public QInternalMimeData
{
public:
    SuppliersBasedMimeData(std::shared_ptr<const std::map<QString, QOhosSupplier<QVariant>>> mimeValuesSuppliers);

protected:
    bool hasFormat_sys(const QString &mimeType) const override;
    QStringList formats_sys() const override;
    QVariant retrieveData_sys(const QString &mimeType, QMetaType type) const override;

private:
    std::shared_ptr<const std::map<QString, QOhosSupplier<QVariant>>> m_mimeValuesSuppliers;
};

SuppliersBasedMimeData::SuppliersBasedMimeData(
    std::shared_ptr<const std::map<QString, QOhosSupplier<QVariant>>> mimeValuesSuppliers)
    : m_mimeValuesSuppliers(mimeValuesSuppliers)
{
}

bool SuppliersBasedMimeData::hasFormat_sys(const QString &mimeType) const
{
    return m_mimeValuesSuppliers->find(mimeType) != m_mimeValuesSuppliers->end();
}

QStringList SuppliersBasedMimeData::formats_sys() const
{
    QStringList formats;
    for (const auto &entry : *m_mimeValuesSuppliers)
        formats.append(entry.first);
    return formats;
}

QVariant SuppliersBasedMimeData::retrieveData_sys(const QString &mimeType, QMetaType) const
{
    auto supplierIter = m_mimeValuesSuppliers->find(mimeType);
    return supplierIter != m_mimeValuesSuppliers->end()
        ? (supplierIter->second)()
        : QVariant();
}

}

QOhosSupplier<std::unique_ptr<QMimeData>> makeQOhosLazyFetchMimeDataFactory(
    std::map<QString, QOhosSupplier<QVariant>> mimeValuesSuppliers)
{
    auto memoizingSuppliers = std::make_shared<std::map<QString, QOhosSupplier<QVariant>>>();
    for (auto &entry : mimeValuesSuppliers)
        memoizingSuppliers->emplace(entry.first, makeMemoizingSupplier(std::move(entry.second)));

    return [memoizingSuppliers]() {
        return std::make_unique<SuppliersBasedMimeData>(memoizingSuppliers);
    };
}

QOhosSupplier<std::unique_ptr<QMimeData>> makeQOhosMimeDataFactory(std::map<QString, QVariant> mimeValues)
{
    auto mimeValuesSuppliers = std::make_shared<std::map<QString, QOhosSupplier<QVariant>>>();
    for (auto &entry : mimeValues) {
        mimeValuesSuppliers->emplace(
            entry.first,
            [value = entry.second]() {
                return value;
            });
    }

    return [mimeValuesSuppliers]() {
        return std::make_unique<SuppliersBasedMimeData>(mimeValuesSuppliers);
    };
}

QT_END_NAMESPACE
