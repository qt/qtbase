// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsdropdataobject.h"

#include <QtCore/qurl.h>
#include <QtCore/qmimedata.h>
#include "qwindowsmimeregistry.h"

QT_BEGIN_NAMESPACE

using namespace Qt::Literals::StringLiterals;

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
QWindowsDropDataObject::GetData(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium) noexcept
{
    if (shouldIgnore(pformatetc))
        return ResultFromScode(DATA_E_FORMATETC);

    return QWindowsOleDataObject::GetData(pformatetc, pmedium);
}

STDMETHODIMP
QWindowsDropDataObject::QueryGetData(LPFORMATETC pformatetc) noexcept
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
        QString formatName = QWindowsMimeRegistry::clipboardFormatName(pformatetc->cfFormat);
        if (pformatetc->cfFormat == CF_UNICODETEXT
                || pformatetc->cfFormat == CF_TEXT
                || formatName == "UniformResourceLocator"_L1
                || formatName == "UniformResourceLocatorW"_L1) {
            const auto urls = dropData->urls();
            return std::all_of(urls.cbegin(), urls.cend(), [] (const QUrl &u) { return u.isLocalFile(); });
        }
    }

    return false;
}

QT_END_NAMESPACE
