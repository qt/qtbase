/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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
#include "qplatformclipboard.h"

#ifndef QT_NO_CLIPBOARD

#include <QtGui/private/qguiapplication_p.h>
#include <QtCore/qmimedata.h>

QT_BEGIN_NAMESPACE

class QClipboardData
{
public:
    QClipboardData();
   ~QClipboardData();

    void setSource(QMimeData* s)
    {
        if (s == src)
            return;
        delete src;
        src = s;
    }
    QMimeData* source()
        { return src; }

private:
    QMimeData* src;
};

QClipboardData::QClipboardData()
{
    src = nullptr;
}

QClipboardData::~QClipboardData()
{
    delete src;
}

Q_GLOBAL_STATIC(QClipboardData,q_clipboardData);

/*!
    \class QPlatformClipboard
    \since 5.0
    \internal
    \preliminary
    \ingroup qpa

    \brief The QPlatformClipboard class provides an abstraction for the system clipboard.
 */

QPlatformClipboard::~QPlatformClipboard()
{

}

QMimeData *QPlatformClipboard::mimeData(QClipboard::Mode mode)
{
    //we know its clipboard
    Q_UNUSED(mode);
    return q_clipboardData()->source();
}

void QPlatformClipboard::setMimeData(QMimeData *data, QClipboard::Mode mode)
{
    //we know its clipboard
    Q_UNUSED(mode);
    q_clipboardData()->setSource(data);

    emitChanged(mode);
}

bool QPlatformClipboard::supportsMode(QClipboard::Mode mode) const
{
    return mode == QClipboard::Clipboard;
}

bool QPlatformClipboard::ownsMode(QClipboard::Mode mode) const
{
    Q_UNUSED(mode);
    return false;
}

void QPlatformClipboard::emitChanged(QClipboard::Mode mode)
{
    if (!QGuiApplicationPrivate::is_app_closing) // QTBUG-39317, prevent emission when closing down.
        QGuiApplication::clipboard()->emitChanged(mode);
}

QT_END_NAMESPACE

#endif //QT_NO_CLIPBOARD
