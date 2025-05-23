// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include "qcomparisontesthelper_p.h"

QT_BEGIN_NAMESPACE

namespace QTestPrivate {

QByteArray formatTypeWithCRefImpl(QMetaType type, bool isConst, bool isRef, bool isRvalueRef)
{
    QByteArray res(type.name());
    if (isConst)
        res.append(" const");
    if (isRef)
        res.append(isRvalueRef ? " &&" : " &");
    return res;
}

} // namespace QTestPrivate

QT_END_NAMESPACE
