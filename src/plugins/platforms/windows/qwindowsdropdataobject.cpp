/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qwindowsdropdataobject.h"

#include <QtCore/qurl.h>
#include <QtCore/qmimedata.h>
#include "qwindowsmime.h"

QT_BEGIN_NAMESPACE

/*!
    \class QWindowsDropDataObject
    \brief QWindowsOleDataObject subclass specialized for handling Drag&Drop.

    Prevents "text/uri-list" data for local files from being exported as text
    or URLs, to allow dropped files to be attached to Office applications
    (instead of creating local hyperlinks).

    \internal
*/

QWindowsDropDataObject::QWindowsDropDataObject(QMimeData *mimeData) :
    QWindowsOleDataObject(mimeData)
{
}

QWindowsDropDataObject::~QWindowsDropDataObject() = default;

STDMETHODIMP
QWindowsDropDataObject::GetData(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium)
{
    if (shouldIgnore(pformatetc))
        return ResultFromScode(DATA_E_FORMATETC);

    return QWindowsOleDataObject::GetData(pformatetc, pmedium);
}

STDMETHODIMP
QWindowsDropDataObject::QueryGetData(LPFORMATETC pformatetc)
{
    if (shouldIgnore(pformatetc))
        return ResultFromScode(DATA_E_FORMATETC);

    return QWindowsOleDataObject::QueryGetData(pformatetc);
}

// If the data is "text/uri-list" only, and all URIs are for local files,
// we prevent it from being exported as text or URLs, to make target applications
// like MS Office attach or open the files instead of creating local hyperlinks.
bool QWindowsDropDataObject::shouldIgnore(LPFORMATETC pformatetc) const
{
    QMimeData *dropData = mimeData();

    if (dropData && dropData->formats().size() == 1 && dropData->hasUrls()) {
        QString formatName = QWindowsMimeConverter::clipboardFormatName(pformatetc->cfFormat);
        if (pformatetc->cfFormat == CF_UNICODETEXT
                || pformatetc->cfFormat == CF_TEXT
                || formatName == QStringLiteral("UniformResourceLocator")
                || formatName == QStringLiteral("UniformResourceLocatorW")) {
            const auto urls = dropData->urls();
            return std::all_of(urls.cbegin(), urls.cend(), [] (const QUrl &u) { return u.isLocalFile(); });
        }
    }

    return false;
}

QT_END_NAMESPACE
