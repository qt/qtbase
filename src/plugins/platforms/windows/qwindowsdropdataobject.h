// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

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
    STDMETHOD(GetData)(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium) override;
    STDMETHOD(QueryGetData)(LPFORMATETC pformatetc) override;

private:
    bool shouldIgnore(LPFORMATETC pformatetc) const;
};

QT_END_NAMESPACE

#endif // QWINDOWSDROPDATAOBJECT_H
