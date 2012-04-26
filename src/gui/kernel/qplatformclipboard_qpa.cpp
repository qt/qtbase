/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
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
    src = 0;
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
    QGuiApplication::clipboard()->emitChanged(mode);
}

QT_END_NAMESPACE

#endif //QT_NO_CLIPBOARD
