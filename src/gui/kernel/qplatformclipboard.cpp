// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
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
