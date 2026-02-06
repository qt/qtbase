// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QWINDOWSDROPDATAOBJECT_H
#define QWINDOWSDROPDATAOBJECT_H

#include "qwindowsole.h"

QT_BEGIN_NAMESPACE

class QWindowsDropDataObject : public QWindowsOleDataObject
{
public:
    explicit QWindowsDropDataObject(QMimeData *mimeData);
    ~QWindowsDropDataObject() override;

    // overridden IDataObject methods
    STDMETHOD(GetData)(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium) noexcept override;
    STDMETHOD(QueryGetData)(LPFORMATETC pformatetc) noexcept override;

private:
    bool shouldIgnore(LPFORMATETC pformatetc) const;
};

QT_END_NAMESPACE

#endif // QWINDOWSDROPDATAOBJECT_H
