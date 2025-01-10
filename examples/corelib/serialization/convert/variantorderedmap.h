// Copyright (C) 2018 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef VARIANTORDEREDMAP_H
#define VARIANTORDEREDMAP_H

#include <QList>
#include <QPair>
#include <QVariant>
#include <QVariantMap>

class VariantOrderedMap : public QList<QPair<QVariant, QVariant>>
{
public:
    VariantOrderedMap() = default;
    VariantOrderedMap(const QVariantMap &map)
    {
        reserve(map.size());
        for (auto it = map.begin(); it != map.end(); ++it)
            append({it.key(), it.value()});
    }
};

#endif // VARIANTORDEREDMAP_H
