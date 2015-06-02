/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QIODEVICE_P_H
#define QIODEVICE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of QIODevice. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "QtCore/qiodevice.h"
#include "QtCore/qbytearray.h"
#include "QtCore/qobjectdefs.h"
#include "QtCore/qstring.h"
#include "private/qringbuffer_p.h"
#ifndef QT_NO_QOBJECT
#include "private/qobject_p.h"
#endif

QT_BEGIN_NAMESPACE

#ifndef QIODEVICE_BUFFERSIZE
#define QIODEVICE_BUFFERSIZE Q_INT64_C(16384)
#endif

Q_CORE_EXPORT int qt_subtract_from_timeout(int timeout, int elapsed);

class Q_CORE_EXPORT QIODevicePrivate
#ifndef QT_NO_QOBJECT
    : public QObjectPrivate
#endif
{
    Q_DECLARE_PUBLIC(QIODevice)

public:
    QIODevicePrivate();
    virtual ~QIODevicePrivate();

    QIODevice::OpenMode openMode;
    QString errorString;

    QRingBuffer buffer;
    qint64 pos;
    qint64 devicePos;
    qint64 transactionPos;
    bool transactionStarted;
    bool baseReadLineDataCalled;

    virtual bool putCharHelper(char c);

    enum AccessMode {
        Unset,
        Sequential,
        RandomAccess
    };
    mutable AccessMode accessMode;
    inline bool isSequential() const
    {
        if (accessMode == Unset)
            accessMode = q_func()->isSequential() ? Sequential : RandomAccess;
        return accessMode == Sequential;
    }

    inline bool isBufferEmpty() const
    {
        return buffer.isEmpty() || (transactionStarted && isSequential()
                                    && transactionPos == buffer.size());
    }
    void seekBuffer(qint64 newPos);

    virtual qint64 peek(char *data, qint64 maxSize);
    virtual QByteArray peek(qint64 maxSize);

#ifdef QT_NO_QOBJECT
    QIODevice *q_ptr;
#endif
};

QT_END_NAMESPACE

#endif // QIODEVICE_P_H
