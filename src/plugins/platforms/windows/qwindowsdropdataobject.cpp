/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwindowsdropdataobject.h"

#include <QtCore/QUrl>
#include <QtCore/QMimeData>

QT_BEGIN_NAMESPACE

/*!
    \class QWindowsDropDataObject
    \brief QWindowsOleDataObject subclass specialized for handling Drag&Drop.

   Only allows "text/uri-list" data to be exported as CF_HDROP, to allow dropped
   files to be attached to Office applications (instead of adding an URL link).

    \internal
    \ingroup qt-lighthouse-win
*/

QWindowsDropDataObject::QWindowsDropDataObject(QMimeData *mimeData) :
    QWindowsOleDataObject(mimeData)
{
}

QWindowsDropDataObject::~QWindowsDropDataObject()
{
}

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

// If the data is text/uri-list for local files, tell we can only export it as CF_HDROP.
bool QWindowsDropDataObject::shouldIgnore(LPFORMATETC pformatetc) const
{
    QMimeData *dropData = mimeData();

    if (dropData && dropData->hasFormat(QStringLiteral("text/uri-list")) && (pformatetc->cfFormat != CF_HDROP)) {
        QList<QUrl> urls = dropData->urls();
        return std::any_of(urls.cbegin(), urls.cend(), [] (const QUrl &u) { return u.isLocalFile(); });
    }

    return false;
}

QT_END_NAMESPACE
